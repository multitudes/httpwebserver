
#include "HTTPServer.hpp"
#include "Constants.hpp"
#include "Parser.hpp"
#include "Responses.hpp"
#include "SocketUtils.hpp"
#include "URLMatcher.hpp"
#include "Utils.hpp"
#include "debug.h"
#include <algorithm>
#include <ctime>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

using std::map;
using std::string;
using std::vector;

namespace HTTPServer {

/**
 * testing: use the following command to test the server
 *
 * nc localhost 4244
 * curl -X POST --data-raw "This is a test" localhost:4244
 * wget -v --post-data="hello world"  http://localhost:4244
 * telnet localhost:4244
 * curl "http://localhost:4244/?sss='this%20is'"
 * curl -H "Content-Type: application/json"
 * "http://localhost:4244/?data=This%20is%20a%20test"
 * curl -X POST -H "Content-Type: application/json" --data-raw '{"message":
 * "This is a test"}' localhost:4244 curl -I -H "Content-Type: application/json"
 * http://localhost:4244
 */

/*
curl -X POST --data-binary @uploadtest.txt http://localhost:4244/upload/test.txt
this will work on the mac - simulate slow connx
curl --limit-rate 1 --verbose http://localhost:4244
*/

// pollfd is an array of pollfd which contain
// the file descriptors to poll and the events we want to monitor

typedef std::vector<struct pollfd> PollfdsVector;
PollfdsVector pollfds;
vector<int> serverSockets;
map<int, HTTPConnxData> connections;
vector<ServerData> configs_;
std::set<pid_t> terminatedPids;

// I will keep them into a map because they are being stored only at the
// beginning of a connection static map<int, string> remoteAddresses;

/**
 * @brief Entrypoint for the HTTP server
 *
 * The configs_ need to be initialized when starting the function
 * and they will be available subsequently in the name space but since
 * it is a singleton and there is no performance issue they will be
 * always available calling Config::getServerData();
 */
int run(std::string configFile) {
  (void)configFile; // Unused variable
  configs_ = Config::getServerData();

  SocketUtils::initialize();

  if (configs_.empty()) {
    debuglog(RED, "No configuration data found");
    throw std::runtime_error("Error: config with empty ports");
  }

  createServerSockets(configs_, serverSockets);

  while (true) {

    // when autoreload is 1 then reload the config file
    // long currentTime = Parser::getCurrentTimeMillis();
    // if (Constants::autoReload) {
    //   if (!reload(configFile, currentTime)) {
    //     exit(EXIT_FAILURE);
    //   }
    // }

    int poll_result =
        poll(&pollfds[0], static_cast<nfds_t>(pollfds.size()), 10000);

    if (poll_result < 0) {
      if (errno != EINTR) {
        perror("poll failed");
        break;
      }
      perror("Poll got signal");
      continue;
    } else if (poll_result == 0) {
      // also a good place to check
      SocketUtils::checkForIdleConnections();
      cleanupClosedConnections();
      continue;
    }

    SocketUtils::checkForIdleConnections();

    // Process events on file descriptors
    for (size_t i = 0; i < pollfds.size(); i++) {

      if (checkPollErrors(pollfds[i])) {
        continue; // Skip to next iteration if no poll or minor errors
      }

      int current_fd = pollfds[i].fd;

      // incoming connection - server socket
      if ((pollfds[i].revents & POLLIN) != 0) {
        // handle connx request to server socket - server will accept the connx
        // and create and add new fd to pool - no need for state for server
        // sockets but will be added for client sockets
        if (gotServerSocketAddNewConnx(pollfds[i].fd)) {
          continue;
        }
      }

      // Now safely get reference - the connection data is in a map
      // and it has been added in accept. the fd could be an fd cgi and
      // it will return the parent connection data -
      // Now safely get reference to the connection data using the new function
      debug("getting connection data for fd %d", current_fd);
      HTTPConnxData* conn_ptr = NULL; // Initialize pointer to NULL
      if (!getConnectionDataByFD(current_fd, conn_ptr)) {
          // Connection not found for this fd, handle error/cleanup
          debug("FD %d not found in connections - removing", current_fd);
          SocketUtils::remove_from_poll(current_fd);
          close(current_fd);
          continue; // Continue to the next fd in pollfds
      }
      // If we reach here, conn_ptr is valid and points to the connection data
      HTTPConnxData &conn = *conn_ptr; // Get a reference for convenience



      debug("conn fd %d state %d", conn.client_fd, conn.state);
      debug("------ current fd %d and is %s", current_fd,
            (pollfds[i].revents & POLLOUT) ? "POLLOUT" : "POLLIN");
      debug("poll size %ld", pollfds.size());
      debug("number of connections %ld", HTTPServer::connections.size());
      
      // Update activity time ONLY when I/O actually happens
      if (pollfds[i].revents & (POLLIN | POLLOUT)) {
        conn.data.lastActivityTime = std::time(NULL);
      }

      /* -------------  CONN_INCOMING  ---------------- */
      if (pollfds[i].revents & POLLIN && conn.state == CONN_INCOMING) {
        debug("got CONN_INCOMING fd %d", conn.client_fd);
        conn.data.client_timeout = 0;
        URLMatcher::validateRequest(conn);
        continue;
      }

      /* ----------- KEEP PARSING_HEADER --------------- */
      if (pollfds[i].revents & POLLIN && conn.state == CONN_PARSING_HEADER) {
        debug("CONN_PARSING_HEADER fd %d", conn.client_fd);
        URLMatcher::validateRequest(conn);
        continue;
      }

      /* -------------  CONN_RECV_CHUNKS  ---------------- */
      if (pollfds[i].revents & POLLIN && conn.state == CONN_RECV_CHUNKS) {
        debug("CONN_RECV_CHUNKS fd %d", conn.client_fd);
        URLMatcher::validateRequest(conn);
        continue;
      }

      /*  -----------  CONN_SIMPLE_RESPONSE -----------  */
      if (pollfds[i].revents & POLLOUT && conn.state == CONN_SIMPLE_RESPONSE) {
        debug("CONN_SIMPLE_RESPONSE fd %d", conn.client_fd);
        debuglog(YELLOW, "Connection fd %d in state SIMPLE_RESPONSE",
                 conn.client_fd);
        if (!conn.finishedSendingSimpleResponse()) {
          continue;
        }
        if (conn.closeConnection) {
          debug("Closing connection %d", conn.client_fd);
          SocketUtils::remove_from_poll(conn.client_fd);
          close(conn.client_fd);
          conn.client_fd = -1; // Mark as closed
        } else {
          debug("Keeping connection alive %d", conn.client_fd);
        }
      }

      /*    -------- FILE REQUEST -----------      */
      if (pollfds[i].revents & POLLOUT && conn.state == CONN_FILE_REQUEST) {
        debug("CONN_FILE_REQUEST client fd %d POLLOUT", conn.client_fd);
        debuglog(YELLOW, "CONN_FILE_REQUEST client fd %d POLLOUT",
                 conn.client_fd);
        if (!conn.settingHeadersIfNeeded()) {
          debug("Failed to set headers for connection %d", conn.client_fd);
          debuglog(RED, "Failed to send headers for connection %d",
                   conn.client_fd);
          continue;
        }
        if (!conn.readNewDataFromFile() || !conn.sendNewDataFromFileToClient()) {
          debuglog(YELLOW, "cound not send data to client for connection %d",
                   conn.client_fd);
          continue;
        }
        conn.checkCompletionConditions();
        if (conn.closeConnection) {
          debug("Closing connection %d", conn.client_fd);
          SocketUtils::remove_from_poll(conn.client_fd);
          close(conn.client_fd);
          conn.client_fd = -1; // Mark as closed
        } else {
          debug("Keeping connection alive %d", conn.client_fd);
        }
        continue;
      }

      /*    -------- UPLOAD -----------      */
      if (conn.state == CONN_UPLOAD) {
        debuglog(YELLOW, "Connection fd %d in state UPLOAD", conn.client_fd);
        debug("CONN_UPLOAD fd %d", conn.client_fd);
        if (conn.writingFirstPayloadCompletesUpload()) {
          continue;
        }
        uploadLoop(conn, pollfds[i]);
      }

      /*    -------- CGI FINISHED -----------      */
      if (conn.state == CONN_CGI_FINISHED) {
        conn.cgiData.buffer.clear();
        conn.cgiData.buffer.resize(0);
        if (conn.cgiData.cgi_stdin_fd != -1) {
          SocketUtils::remove_from_poll(conn.cgiData.cgi_stdin_fd);
        }
        if (conn.cgiData.cgi_stdout_fd != -1) {
          SocketUtils::remove_from_poll(conn.cgiData.cgi_stdout_fd);
        }
        conn.reset(); // todo check if pid not reset
        // SocketUtils::remove_from_poll(conn.client_fd);
        if (conn.errorStatus != 0) {
          debug("Will send error response %d", conn.errorStatus);
          Responses::htmlErrorResponse(conn, conn.errorStatus);
          conn.errorStatus = 0;
          conn.closeConnection = true;
        } else {
          debug("CGI finished but kept alive %d", conn.client_fd);
        }
        break;
      }

      /*    -------- CONN_CGI_INCOMING -----------      */
      if (conn.state == CONN_CGI_INCOMING) {
        debuglog(YELLOW, "Connection fd %d in state CGI", conn.client_fd);
        debug("CONN_CGI_INCOMING; - current fd %d and is %s", current_fd,
              (pollfds[i].revents & POLLOUT) ? "POLLOUT" : "POLLIN");
        debug("poll_result %d", poll_result);
        debug("CONN_CGI_INCOMING; fd %d", conn.client_fd);
        debug("CGI fd in %d", conn.cgiData.cgi_stdin_fd);
        debug("CGI fd out %d", conn.cgiData.cgi_stdout_fd);

        if (current_fd == conn.client_fd && (pollfds[i].revents & POLLIN) &&
            conn.cgiData.buffer.empty()) {
          // first read from the client
          debug("POLLIN event on client fd %d", conn.client_fd);
          for (size_t j = 0; j < pollfds.size(); j++) {
            if (pollfds[j].fd == conn.cgiData.cgi_stdin_fd &&
                (pollfds[j].revents & POLLOUT)) {
              debug("POLLOUT event on CGI stdin fd %d",
                    conn.cgiData.cgi_stdin_fd);
              // found! reset the timeout
              conn.cgiData.child_timeout = 0;
              // read from client
              conn.read_from_client_into_buffer();
              break;
            }
          }
          // set the timeout because not found
          //              conn.cgiData.child_timeout = 0;
          if (conn.check_for_child_timeout()) {
            break;
          }
        }

        // check if the child process is pollout ready to be written to
        // and i have data in buffer from the preparecgi function
        if (!conn.cgiData.buffer.empty()) {
          debug("cgiData is receiving");
          for (size_t j = 0; j < pollfds.size(); j++) {
            if (pollfds[j].fd == conn.cgiData.cgi_stdin_fd &&
                (pollfds[j].revents & POLLOUT)) {
              // found! reset the timeout
              conn.cgiData.child_timeout = 0;
              debug("POLLOUT event on CGI stdin fd %d",
                    conn.cgiData.cgi_stdin_fd);
              conn.cgiData.child_timeout = 0;
              // write to cgi the buffer if not empty
              conn.write_to_child_stdin(current_fd, pollfds[j].fd);
              break; // whatever happens to the state we break the for loop
                     // because we found the fd we were looking for
            } // end -> if (pollfds[j].fd == conn.cgiData.cgi_stdin_fd &&
          } // end for loop
          if (conn.check_for_child_timeout()) {
            break;
          }
        }
      }

      /*    -------- CGI SENDING -----------      */
      if (conn.state == CONN_CGI_SENDING) {
        // Handle data FROM CGI process (ready to write to client from cgi)
        // my client is ready to be written to
        if (current_fd == conn.client_fd && (pollfds[i].revents & POLLOUT)) {
          debuglog(YELLOW, "Connection fd %d in state CGI SENDING",
                   conn.client_fd);
          debug("CONN_CGI_SENDING fd %d", conn.client_fd);
          debug("cgiData is sending  and client fd %d is POLLOUT",
                conn.client_fd);
          // before to read from child i check if i have a buffer leftover
          conn.write_to_client_from_cgi();

          // after writing the excess buffer i need to read from the cgi
          for (size_t j = 0; j < pollfds.size(); j++) {
            // and the cgi process is ready to be read from
            if (pollfds[j].fd == conn.cgiData.cgi_stdout_fd &&
                (pollfds[j].revents & POLLIN)) {
              debug("POLLIN event on CGI stdout fd %d",
                    conn.cgiData.cgi_stdout_fd);
              // reset timeout
              conn.cgiData.child_timeout = 0;
              // read-write to client from cgi
              conn.cgiData.buffer.resize(Constants::BUFFER_SIZE);
              ssize_t bytes_read =
                  ::read(conn.cgiData.cgi_stdout_fd, &conn.cgiData.buffer[0],
                         conn.cgiData.buffer.size());
              if (bytes_read < 0) {
                perror("Failed to read from CGI stdout");
                conn.state = CONN_CGI_FINISHED;
                conn.errorStatus = 500;
                conn.closeConnection = true;
                break;
              } else if (bytes_read == 0) {
                debug("CGI process finished");
                conn.state = CONN_CGI_FINISHED;
                break;
              }
              debug("Received %ld bytes from CGI stdout", bytes_read);
              debuglog(MAGENTA, "response from CGI: %s",
                         conn.cgiData.buffer.c_str());
              
              if (conn.sendCgiDataToClient(bytes_read) == false) {
                debug("Failed/finished to send data to client");
                conn.state = CONN_CGI_FINISHED;
              }
              // if (pollfds[j].fd == conn.cgiData.cgi_stdout_fd &&
              //   (pollfds[j].revents & POLLHUP)) {
              //     throw std::runtime_error(
              //         "POLLHUP event on CGI stdout fd " +
              //         Utils::to_string(conn.cgiData.cgi_stdout_fd));
              //   debug("POLLHUP event on CGI stdout fd %d",
              //         conn.cgiData.cgi_stdout_fd);
              //     conn.state = CONN_CGI_FINISHED;
              //   }
              break;
            }
          }
          if (conn.check_for_child_timeout()) {
            break;
          }
        }
      } // end of the state cgi_sending check
      conn.check_for_client_timeout();
    } // end of the main for loop in pollfds
    cleanupClosedConnections();
  }
  return 0;
}

/**
 * @brief create bind listen sockets for the server
 *
 * This function creates server sockets for each port in the configuration
 * and adds them to the poll vector. It also sets the server sockets to
 * non-blocking mode and sets the timeout for the client sockets.
 */
void createServerSockets(const vector<ServerData> &configs,
                         vector<int> &serverSockets) {
  for (size_t i = 0; i < configs.size(); i++) {
    for (size_t j = 0; j < configs[i].ports.size(); j++) {
      int server_fd;
      if ((server_fd = SocketUtils::createBindSocket(configs[i].ports[j])) <
          0) {
        perror("Error creating socket");
        throw std::runtime_error("Socket creation failed");
      }
      debuglog(YELLOW, "Socket created with fd %d", server_fd);
      if (!SocketUtils::listenSocket(server_fd)) {
        perror("Error listening on socket");
        close(server_fd);
        throw std::runtime_error("Error listening on socket");
      }
      serverSockets.push_back(server_fd);
      SocketUtils::add_to_poll(server_fd, POLLIN);
      debuglog(GREEN, "Server listening on port %d", configs[i].ports[j]);
    }
  }
}



/**
 * @brief Reload the configuration file called by reload
 *
 * It is a throwable function
 */
void reloadConfigFile(std::string configFile, vector<int> &serverSockets,
                      vector<ServerData> &configs_) {
  SocketUtils::shutdownServer();
  SocketUtils::initialize();
  Config::cleanup();
  Config::initialize(configFile);
  configs_ = Config::getServerData();
  if (configs_.empty()) {
    debuglog(RED, "No configuration data found");
    throw std::runtime_error("Error: config with empty ports");
  }

  createServerSockets(configs_, serverSockets);

  debuglog(GREEN, "Configuration reload complete with %zu servers",
           Config::getServerData().size());
}

/**
 * @brief Reload the configuration file if needed
 *
 * Used when we set the autoreload option in the config file
 */
bool reload(string configFile, long currentTime) {
  if (currentTime - Parser::starttime > 5000) {
    debuglog(GREEN, "Reloading configuration file %s\n\n", configFile.c_str());
    Parser::starttime = currentTime;
    try {
      reloadConfigFile(configFile, HTTPServer::serverSockets,
                       HTTPServer::configs_);
    } catch (const std::exception &e) {
      debuglog(RED, "Error reloading configuration: %s", e.what());
      return false;
    }
  }
  return true;
}

/**
 * @brief Check for errors on the pollfd
 *
 * return true will continue to the next iteration of the loop
 */
bool checkPollErrors(pollfd currentfd) {
  if (!(currentfd.revents & (POLLIN | POLLOUT))) {
    return true; // No events on this fd
  }
  if (SocketUtils::gotPollhupShouldSkip(currentfd) ||
      SocketUtils::gotPollerrShouldSkip(currentfd)) {
    return true;
  }

  return false; // No errors
}

// Function to check if the pollfd is a server socket and handle the connection
bool gotServerSocketAddNewConnx(int fd) {
  vector<int>::iterator it =
      std::find(serverSockets.begin(), serverSockets.end(), fd);
  if (it != serverSockets.end()) {
    // i got a server socket fd - *it is the fd and I get the index
    // in the server config array and accept that connection
    acceptNewClient(*it);
    return true;
  }
  return false;
}

/**
 * @brief Accept a new client connection
 * 
 * Sets the client socket to non-blocking mode and sets the timeout
 * for the client socket on send and receive. It also checks for
 * maximum connections and accepts the new connection.
 */
void acceptNewClient(int server_fd) {
  int client_fd;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  while (true) {
    client_fd =
        ::accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr),
                 &client_len);
    if (client_fd == -1) {
      // because we use non blocking sockets if i get EWOULDBLOCK it is
      // not an error - it just means there are no more connections to accept
      if (errno != EWOULDBLOCK) {
        debug("accept error: %s\n", strerror(errno));
      }
      break;
    }
    // max connection check!
    if (!maxConnectionsCheck(client_fd)) {
      debug("Max connections reached, rejecting new connection");
      send_critical_error(client_fd, 500);
      close(client_fd);
      continue;
    }

    // set the timeout for the client socket on send and receive
    //   setSendRecTimeout(client_fd);
    if (!SocketUtils::setSendRecTimeout(client_fd)) {
      perror("Failed to set send/receive timeout");
      debug("Failed to set send/receive timeout");
      send_critical_error(client_fd, 500);
      close(client_fd);
      continue;
    }

    // Get the local address of the accepted socket and print it
    // for debugging purposes
    if (!SocketUtils::printLocalAddress(client_fd)) {
      debug("Failed to get local address");
      send_critical_error(client_fd, 500);
      close(client_fd);
      continue;
    }
    debug("New connection from %s:%d", inet_ntoa(client_addr.sin_addr),
          ntohs(client_addr.sin_port));

    HTTPConnxData &conn = connections[client_fd];
    conn.client_fd = client_fd;
    SocketUtils::add_to_poll(client_fd, POLLIN | POLLOUT);
    conn.state = CONN_INCOMING;

    // Store client IP address
    SocketUtils::custom_inet_ntop(AF_INET, &client_addr.sin_addr,
                                  conn.client_ip, sizeof(conn.client_ip));
    uint16_t client_port = ntohs(client_addr.sin_port);

    debuglog(YELLOW, "Incoming client connected from %s:%d", conn.client_ip,
             client_port);
    debuglog(YELLOW,
             "Connection data initialized in state INCOMING for client %d",
             client_fd);
    debug("Connection data initialized in state INCOMING for client %d",
                   client_fd);
  }
}

/**
 * @brief Set the send and receive timeouts for a client socket
 *
 * @param conf The server configuration
 * @param clientfd The client socket file descriptor
 *
 * I use the values in the server configuration to set the send and receive
 * timeouts for the client socket. This is useful for handling slow or
 * unresponsive clients.
 */
void setSendRecTimeout(int clientfd) {
  // connections timeout rcvd
  struct timeval tv;
  tv.tv_sec = Constants::requestTimeout;
  tv.tv_usec = 0;
  if (::setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    debug("setsockopt SO_RCVTIMEO failed");
    send_critical_error(clientfd, 500);
    close(clientfd);
  }

  // Set send timeout
  tv.tv_sec = Constants::responseTimeout;
  if (::setsockopt(clientfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
    debug("setsockopt SO_SNDTIMEO failed"); // todo check if this works in cgi
    send_critical_error(clientfd, 500);
    close(clientfd);
  }
}

/**
 * @brief Max connection check
 *
 * @param clientfd The client socket file descriptor
 *
 * In the server conf we have the max connections allowed for each server if
 * specified or a default value. We dont allow new connections in the poll loop
 * if the max connections are reached.
 */
bool maxConnectionsCheck(int clientfd) {

  if (pollfds.size() >= static_cast<size_t>(Constants::maxConnections)) {
    debug("Maximum connections reached, rejecting new connection");
    send_critical_error(clientfd, 503);
    close(clientfd);
    return false;
  }
  return true;
}

/** 
 * @brief Upload loop for handling file uploads
 * 
 * This function is called when the connection is in the UPLOAD state.
 * It checks for incoming data from the client and writes it to the file.
 * If the upload is complete, it calls the uploadComplete function.
 */
void uploadLoop(HTTPConnxData &conn, pollfd currentfd) {
  if (currentfd.revents & POLLIN) {
    debug("POLLIN event on upload connection %d", conn.client_fd);
    debuglog(YELLOW, "Handling upload event for connection %d", conn.client_fd);

    if (!conn.readFromClientForUpload() || !conn.writeUploadToFile()) {
      return;
    }
    conn.uploadComplete();
  }
}

/**
 * @brief send error and close the connection
 *
 * include the Connection: close header in the response to inform the client.
 * This is when the headers are not yet received. In this case
 * I cannot send custom error pages. Example: malformed requests.
 */
void send_critical_error(int fd, int code) {
  std::string response = "HTTP/1.1 " + Utils::to_string(code) + " " +
                         Constants::statusMessages[code] +
                         "\r\n"
                         "Connection: close\r\n"
                         "Content-Length: 0\r\n"
                         "\r\n";
  debug("Sending the error response %s", response.c_str());
  // i dont check for errors here because the connection will be closed
  ::send(fd, response.c_str(), response.size(), MSG_NOSIGNAL);
}

/**
 * @brief Finds the connection data associated with a given file descriptor.
 *
 * Searches the connections map first by client_fd (the map key).
 * If not found, iterates through all connections to check if the fd
 * matches a CGI pipe fd (stdin or stdout).
 *
 * @param fd The file descriptor to search for.
 * @param out_conn_ptr A reference to a pointer. If the connection is found,
 *                     this pointer will be set to point to the found
 *                     HTTPConnxData object. Otherwise, it might be NULL.
 * @return true if a connection associated with the fd was found, false otherwise.
 */
bool getConnectionDataByFD(int fd, HTTPConnxData*& out_conn_ptr) {
  // Try finding by client_fd (map key) first
  std::map<int, HTTPConnxData>::iterator conn_it = HTTPServer::connections.find(fd);
  if (conn_it != HTTPServer::connections.end()) {
      out_conn_ptr = &(conn_it->second); // Set the output pointer
      return true;                       // Found
  }

  // If not found by client_fd, check CGI pipe fds
  for (std::map<int, HTTPConnxData>::iterator it = HTTPServer::connections.begin();
       it != HTTPServer::connections.end(); ++it) {
      if (it->second.cgiData.cgi_stdin_fd == fd ||
          it->second.cgiData.cgi_stdout_fd == fd) {
          out_conn_ptr = &(it->second); // Set the output pointer
          return true;                  // Found
      }
  }

  // Not found anywhere
  out_conn_ptr = NULL; // Explicitly set to NULL if not found
  return false;
}

/**
 * @brief Iterates through the connections map and erases entries marked for removal.
 *
 * Connections are marked for removal by setting their client_fd member to -1.
 * This function safely removes such entries from the global connections map.
 */
void cleanupClosedConnections() {
  std::map<int, HTTPConnxData>::iterator cleanup_it = HTTPServer::connections.begin();
  while (cleanup_it != HTTPServer::connections.end()) {
      // Check if the connection is marked for removal
      if (cleanup_it->second.client_fd == -1) {
          int original_fd = cleanup_it->first; // Get original fd for logging before erase
          debuglog(YELLOW, "Erasing connection object marked for removal (originally fd %d)", original_fd);
          // Ensure reset() was called before setting client_fd = -1
          // C++98 way to erase from map while iterating:
          std::map<int, HTTPConnxData>::iterator to_erase = cleanup_it; // Store iterator to erase
          ++cleanup_it; // Advance the main iterator FIRST
          HTTPServer::connections.erase(to_erase); // Erase the old position
      } else {
          // Move to the next element if not erasing
          ++cleanup_it;
      }
  }
}

} // namespace HTTPServer
