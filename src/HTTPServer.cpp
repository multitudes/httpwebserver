
#include "HTTPServer.hpp"
#include "Constants.hpp"
#include "DirectoryListing.hpp"
#include "Parser.hpp"
#include "Responses.hpp"
#include "SocketUtils.hpp"
#include "URLMatcher.hpp"
#include "Utils.hpp"
#include "debug.h"
#include <algorithm>
#include <cassert>
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
map<int, std::time_t> lastActivityTime;
vector<ServerData> configs_;

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
  configs_ = Config::getServerData();

  SocketUtils::initialize();

  if (configs_.empty()) {
    debuglog(RED, "No configuration data found");
    throw std::runtime_error("Error: config with empty ports");
  }

  createServerSockets(configs_, serverSockets);

  while (true) {

    // when autoreload is 1 then reload the config file
    long currentTime = Parser::getCurrentTimeMillis();
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

      // Now safely get reference
      HTTPConnxData &conn = getConnectionData(current_fd);

      // Update activity time ONLY when I/O actually happens
      if (pollfds[i].revents & (POLLIN | POLLOUT)) {
        lastActivityTime[current_fd] = std::time(NULL);
      }

      /* -------------  CONN_INCOMING  ---------------- */
      if (pollfds[i].revents & POLLIN && conn.state == CONN_INCOMING) {
        debug("got CONN_INCOMING fd %d", conn.client_fd);
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
        if (!finishedSendingSimpleResponse(conn)) {
          continue;
        }
      }

      /*    -------- FILE REQUEST -----------      */
      if (pollfds[i].revents & POLLOUT && conn.state == CONN_FILE_REQUEST) {
        debug("CONN_FILE_REQUEST client fd %d POLLOUT", conn.client_fd);
        debuglog(YELLOW, "CONN_FILE_REQUEST client fd %d POLLOUT",
                 conn.client_fd);
        if (!settingHeadersIfNeeded(conn)) {
          debug("Failed to set headers for connection %d", conn.client_fd);
          debuglog(RED, "Failed to send headers for connection %d",
                   conn.client_fd);
          continue;
        }
        if (!readNewDataFromFile(conn) || !sendNewDataFromFileToClient(conn)) {
          debuglog(YELLOW, "cound not send data to client for connection %d",
                   conn.client_fd);
          continue;
        }
        checkCompletionConditions(conn);
        continue;
      }

      /*    -------- UPLOAD -----------      */
      if (conn.state == CONN_UPLOAD) {
        debuglog(YELLOW, "Connection fd %d in state UPLOAD", conn.client_fd);
        debug("CONN_UPLOAD fd %d", conn.client_fd);
        if (writingFirstPayloadCompletesUpload(conn)) {
          continue;
        }
        uploadLoop(conn, pollfds[i]);
      }

      /*    -------- CGI FINISHED -----------      */
      if (conn.state == CONN_CGI_FINISHED) {
        SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
        SocketUtils::remove_from_poll(
            conn.cgiData.child_stdout_pipe[0]);
        lastActivityTime.erase(conn.client_fd);
        conn.reset();
        break;
      }

      /*    -------- CGI -----------      */
      if (conn.state == CONN_CGI) {
        debuglog(YELLOW, "Connection fd %d in state CGI", conn.client_fd);
        debug("CONN_CGI - current fd %d and is %s", current_fd,
              (pollfds[i].revents & POLLOUT) ? "POLLOUT" : "POLLIN");

        debug("poll_result %d", poll_result);
        debug("CONN_CGI fd %d", conn.client_fd);
        debug("CGI fd in %d", conn.cgiData.child_stdin_pipe[1]);
        debug("CGI fd out %d", conn.cgiData.child_stdout_pipe[0]);

        // check if the child process is pollout ready to be written to  
        // and i have data in buffer from the preparecgi function
        if (conn.cgiData.is_receiving && !conn.cgiData.buffer.empty()) {
          debug("cgiData is receiving");
          for (size_t j = 0; j < pollfds.size(); j++) {
            if (pollfds[j].fd == conn.cgiData.child_stdin_pipe[1] &&
                (pollfds[j].revents & POLLOUT)) {
              debug("POLLOUT event on CGI stdin fd %d",
                    conn.cgiData.child_stdin_pipe[1]);

              // write to cgi the buffer if any remaining from the
              // initialisation
              ssize_t bytes_written = ::write(conn.cgiData.child_stdin_pipe[1],
                                              conn.cgiData.buffer.c_str(),
                                              conn.cgiData.buffer.size());
              debug("Wrote %ld bytes to CGI stdin", bytes_written);

              if (bytes_written < 0) {
                perror("Failed to write to CGI stdin");
                debug("Failed to write to CGI stdin");
                conn.state = CONN_CGI_FINISHED;
              } else if (bytes_written == 0) {
                // Should not happen with blocking write unless size was 0
                debuglog(YELLOW, "Wrote 0 bytes to CGI stdin (buffer size: %zu)", conn.cgiData.buffer.size());
                debuglog(RED, "Wrote 0 bytes to CGI stdin unexpectedly.");
                conn.state = CONN_CGI_FINISHED;
              } else if (static_cast<size_t>(bytes_written) < conn.cgiData.buffer.size()) {
                // Partial write: Remove written data and wait for next POLLOUT
                debug("Partial write: Wrote %ld bytes to CGI stdin (buffer size: %zu)", bytes_written, conn.cgiData.buffer.size());
                conn.cgiData.buffer.erase(0, static_cast<std::string::size_type>(bytes_written));
                conn.cgiData.bytes_received += static_cast<size_t>(bytes_written);
                // stay in the same state, poll will trigger again
              } else {
                // Full write (bytes_written == conn.cgiData.buffer.size())
                debugcolor(MAGENTA, "wrote request buffer to CGI: %s", conn.cgiData.buffer.c_str()); // Log data before clearing
                // TODO check the bytes received
                conn.cgiData.bytes_received += static_cast<size_t>(bytes_written);
                if (conn.cgiData.bytes_received >= conn.data.content_length) {
                    debug("Full write: Wrote %ld bytes to CGI stdin", bytes_written);
                    // If we have written all data, clear the buffer
                    conn.cgiData.buffer.clear();
                    conn.cgiData.is_receiving = false; // No more data to send
                    conn.cgiData.is_sending = true;    // Data to receive from CGI stdout
                  }
              }
              break; // whatever happens to the state we break the for loop because we found 
                    // the fd we were looking for
              
            } // end -> if (pollfds[j].fd == conn.cgiData.child_stdin_pipe[1] &&

          } // end for loop

        } else if (conn.cgiData.is_receiving && current_fd == conn.client_fd &&
                   (pollfds[i].revents & POLLIN)) {
          // first read from the client
          debug("POLLIN event on client fd %d", conn.client_fd);
          for (size_t j = 0; j < pollfds.size(); j++) {
            // and the cgi process is ready to be read from
            // if (pollfds[j].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            //   perror("Failed to write to CGI stdin");
            //   SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
            //   SocketUtils::remove_from_poll(
            //       conn.cgiData.child_stdout_pipe[0]);
            //   lastActivityTime.erase(conn.client_fd);
            //   conn.reset();
            //   break;
            // }
            if (pollfds[j].fd == conn.cgiData.child_stdin_pipe[1] &&
                (pollfds[j].revents & POLLOUT)) {
              debug("POLLOUT event on CGI stdin fd %d",
                    conn.cgiData.child_stdin_pipe[1]);

              // read from client
              char buffer[BUFFER_SIZE + 1];
              ssize_t bytes_read =
                  ::recv(conn.client_fd, buffer, BUFFER_SIZE, 0);
              if (bytes_read < 0) {
                perror("Failed to read from client");
                SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
                SocketUtils::remove_from_poll(
                    conn.cgiData.child_stdout_pipe[0]);
                lastActivityTime.erase(conn.client_fd);
                conn.reset();

                break;
              } else if (bytes_read == 0) {
                debug("Client closed connection - giving EOF to CGI stdin");
                close(conn.cgiData.child_stdin_pipe[1]);
                SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
                conn.cgiData.is_receiving = false;
                conn.cgiData.is_sending = true;
                buffer[0] = '\0'; //

              } else {

                debug("Received %ld bytes from client", bytes_read);
                debug("writing to cgi stdin %d",
                      conn.cgiData.child_stdin_pipe[1]);

                buffer[bytes_read] = '\0'; // Null-terminate the buffer

                // now write to cgi the buffer
                ssize_t bytes_written =
                    ::write(conn.cgiData.child_stdin_pipe[1], buffer,
                            static_cast<size_t>(bytes_read));

                if (bytes_written < 0) {
                  perror("Failed to write to CGI stdin");
                  SocketUtils::remove_from_poll(
                      conn.cgiData.child_stdin_pipe[1]);
                  SocketUtils::remove_from_poll(
                      conn.cgiData.child_stdout_pipe[0]);
                  lastActivityTime.erase(conn.client_fd);
                  conn.reset();
                  break;
                } else if (bytes_written <
                           BUFFER_SIZE - conn.data.headers_end) {
                  debug("Wrote %ld bytes to CGI stdin", bytes_written);
                  debugcolor(MAGENTA, "request to CGI: %s",
                             conn.cgiData.buffer.c_str());
                  // i finished sending the data to the cgi
                  // close the write end of the pipe to signal EOF to the CGI
                  debuglog(YELLOW, "Closing write end of pipe");
                  SocketUtils::remove_from_poll(
                      conn.cgiData.child_stdin_pipe[1]);
                  close(conn.cgiData.child_stdin_pipe[1]);
                  conn.cgiData.is_receiving = false;
                  conn.cgiData.is_sending = true;
                  conn.cgiData.buffer.clear();
                  break;
                } else if (bytes_written <
                           static_cast<ssize_t>(conn.cgiData.buffer.size())) {
                  // Not all data was written, handle partial write
                  debuglog(YELLOW, "Partial write to CGI stdin");
                  conn.cgiData.buffer.erase(
                      0, static_cast<std::string::size_type>(bytes_written));
                  break;
                } else if (bytes_written ==
                           static_cast<ssize_t>(conn.cgiData.buffer.size())) {
                  // All data was written, clear the buffer
                  conn.cgiData.buffer.clear();
                  break;
                } else if (bytes_written == 0) {
                  // No data was written, this should not happen
                  debuglog(RED, "No data written to CGI stdin");
                  SocketUtils::remove_from_poll(
                      conn.cgiData.child_stdin_pipe[1]);
                  SocketUtils::remove_from_poll(
                      conn.cgiData.child_stdout_pipe[0]);
                  lastActivityTime.erase(conn.client_fd);
                  conn.reset();
                  break;
                }
              }
            }
          }
        }

        // Handle data FROM CGI process (ready to write to client from cgi)
        // my client is ready to be written to
        if (conn.cgiData.is_sending && current_fd == conn.client_fd &&
            (pollfds[i].revents & POLLOUT)) {
          debug("cgiData.is_sending  and client fd %d is POLLOUT",
                conn.client_fd);
          for (size_t j = 0; j < pollfds.size(); j++) {
            // and the cgi process is ready to be read from
            if (pollfds[j].fd == conn.cgiData.child_stdout_pipe[0] &&
                (pollfds[j].revents & POLLIN)) {

              debug("POLLIN event on CGI stdout fd %d",
                    conn.cgiData.child_stdout_pipe[0]);
              // write to client from cgi
              char buffer[BUFFER_SIZE + 1];
              ssize_t bytes_read = ::read(conn.cgiData.child_stdout_pipe[0],
                                          buffer, BUFFER_SIZE);
              if (bytes_read < 0) {
                perror("Failed to read from CGI stdout");
                SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
                SocketUtils::remove_from_poll(
                    conn.cgiData.child_stdout_pipe[0]);
                lastActivityTime.erase(conn.client_fd);
                conn.reset();
                // todo send the error to the client
                break;
              }
              buffer[bytes_read] = '\0'; // Null-terminate the buffer
              debug("Received %ld bytes from CGI stdout", bytes_read);
              debugcolor(MAGENTA, "response from CGI: %s", buffer);
              if (bytes_read == 0 || bytes_read < BUFFER_SIZE) {
                debug("CGI process finished");
                // SocketUtils::remove_from_poll(conn.cgiData.child_stdout_pipe[0]);
                // conn.reset();

                // break;
              }

              // Send data to client
              ssize_t bytes_written = ::send(
                  conn.client_fd, buffer, static_cast<size_t>(bytes_read), 0);
              if (bytes_written < 0) {
                perror("Failed to send data to client");
                close(conn.cgiData.child_stdout_pipe[0]);
                lastActivityTime.erase(conn.client_fd);
                conn.reset();
                continue;
              }
              debug("Sent %ld bytes to client", bytes_written);
              if (bytes_written < BUFFER_SIZE || bytes_written == 0) {
                // i finished sending the data to the client
                // close the read end of the pipe to signal EOF to the CGI
                debuglog(YELLOW, "Closing read end of pipe");
                SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
                SocketUtils::remove_from_poll(
                    conn.cgiData.child_stdout_pipe[0]);
                lastActivityTime.erase(conn.client_fd);
                conn.reset();
              }

              break;
            }
          }
        }



      } // end of the state cgi check

    } // end of the main for loop in pollfds
  }

  // Cleanup
  // TODO
  return 0;
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
  if (SocketUtils::gotPollhupShouldSkip(currentfd) || \
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
      send_critical_error(client_fd, 503);
      debug("Max connections reached, rejecting new connection");
      continue;
    }

    // set the timeout for the client socket on send and receive
    //   setSendRecTimeout(client_fd);
    if (!SocketUtils::setSendRecTimeout(client_fd)) {
      perror("Failed to set send/receive timeout");
      send_critical_error(client_fd, 500);
      debug("Failed to set send/receive timeout");
      continue;
    }

    // Get the local address of the accepted socket and print it
    // for debugging purposes
    if (!SocketUtils::printLocalAddress(client_fd)) {
      send_critical_error(client_fd, 500);
      debug("Failed to get local address");
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

    // add the timeout for the client
    lastActivityTime[client_fd] = std::time(NULL);

    debuglog(YELLOW, "Incoming client connected from %s:%d", conn.client_ip,
             client_port);
    debuglog(YELLOW,
             "Connection data initialized in state INCOMING for client %d",
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

  if (pollfds.size() >= Constants::maxConnections) {
    debug("Maximum connections reached, rejecting new connection");
    send_critical_error(clientfd, 503);
    close(clientfd);
    return false;
  }
  return true;
}

/**
 *
 */
HTTPConnxData &getConnectionData(int fd) {
  // First try to find as normal connection
  std::map<int, HTTPConnxData>::iterator conn_it = connections.find(fd);

  // If not found, check for its CGI pipes
  if (conn_it == connections.end()) {
    for (std::map<int, HTTPConnxData>::iterator it = connections.begin();
         it != connections.end(); ++it) {
      if (it->second.cgiData.child_stdin_pipe[1] == fd ||
          it->second.cgiData.child_stdout_pipe[0] == fd) {
        conn_it = it;
        break;
      }
    }
    // Still not found?
    if (conn_it == connections.end()) {
      debuglog(RED, "FD %d not found in connections", fd);
      close(fd);
      SocketUtils::remove_from_poll(fd);
      lastActivityTime.erase(fd);
    }
  }

  return conn_it->second;
}

/**
 * @brief Check if the upload is complete
 *
 * @param conn The connection data
 * @return true if the upload is complete, false otherwise
 *
 * This function checks if the number of bytes sent is greater than or equal to
 * the content length. If so, it resets the connection and sends a response to
 * the client.
 */
bool uploadComplete(HTTPConnxData &conn) {
  if (conn.data.bytes_sent >= conn.data.content_length) {
    debug("Upload complete");
    lastActivityTime.erase(conn.client_fd);
    conn.reset();
    Responses::createResponse(conn, "text/plain", "File uploaded successfully.",
                              201);
    conn.state = CONN_SIMPLE_RESPONSE;
    return true;
  }
  return false;
}

/**
 * @brief Check if writing the first payload completes the upload
 *
 * @param conn The connection data
 * @return true if the upload is complete, false otherwise
 *
 * This function checks if there is any leftover payload data from the header
 * parsing. If so, it writes that data to the file and checks if the upload is
 * complete.
 */
bool writingFirstPayloadCompletesUpload(HTTPConnxData &conn) {
  if (!conn.data.response.empty()) {
    debug("Writing leftover payload for connection %d", conn.client_fd);
    debuglog(YELLOW, "writing leftover payload for client %d", conn.client_fd);
    ssize_t bytes_written = write(conn.file_fd, conn.data.response.c_str(),
                                  conn.data.response.size());
    if (bytes_written <= 0) {
      perror(bytes_written < 0 ? "Failed to write to file"
                               : "No data written to file");
      lastActivityTime.erase(conn.client_fd);
      conn.reset();
      return true;
    }
    conn.data.bytes_sent += static_cast<size_t>(bytes_written);
    conn.data.response.clear();

    if (uploadComplete(conn)) {
      return true;
      ;
    }
  }
  return false;
}

bool readFromClientForUpload(HTTPConnxData &conn) {
  conn.data.buffer.resize(BUFFER_SIZE);
  ssize_t bytes_read = ::recv(conn.client_fd, conn.data.buffer.data(),
                              conn.data.buffer.size(), MSG_DONTWAIT);
  if (bytes_read <= 0) {
    if (bytes_read == 0) {
      debug("Client disconnected during upload");
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      debug("No data available yet - keep in reading state");
      return false;
    }
    debug("%s", strerror(errno));
    perror("recv failed during upload");
    lastActivityTime.erase(conn.client_fd);
    conn.reset();
    return false;
  }
  // Resize the buffer to the actual amount of data read
  conn.data.buffer.resize(static_cast<size_t>(bytes_read));

  debug("Received %ld bytes from client", bytes_read);
  return true;
}

bool writeUploadToFile(HTTPConnxData &conn) {
  ssize_t bytes_written =
      write(conn.file_fd, conn.data.buffer.data(), conn.data.buffer.size());
  if (bytes_written <= 0) {
    perror(bytes_written < 0 ? "Failed to write to file"
                             : "No data written to file");
    conn.reset();
    return false;
  }
  conn.data.bytes_sent += static_cast<size_t>(bytes_written);
  debug("Wrote %ld bytes to file", bytes_written);
  debug("total bytes sent %zu/%zu", conn.data.bytes_sent,
        conn.data.content_length);
  conn.data.buffer.clear();
  return true;
}

bool finishedSendingSimpleResponse(HTTPConnxData &conn) {
  conn.data.response.reserve(BUFFER_SIZE);
  ssize_t bytes_sent = ::send(conn.client_fd, conn.data.response.c_str(),
                              conn.data.response.size(), 0);
  if (bytes_sent < 0) {
    perror("Failed to send simple response");
    SocketUtils::remove_from_poll(conn.client_fd);
    lastActivityTime.erase(conn.client_fd);
    conn.reset();
  } else if (bytes_sent == 0) {
    debug("No data sent to client %d", conn.client_fd);
  } else {
    debug("Sent %ld bytes to client %d", bytes_sent, conn.client_fd);
    // defensive programming - handle partial send
    // Remove sent bytes from buffer
    conn.data.response.erase(conn.data.response.begin(),
                             conn.data.response.begin() + bytes_sent);
    debug("Sent %zd bytes (%zu remaining in buffer)", bytes_sent,
          conn.data.buffer.size());
    if (!conn.data.response.empty()) {
      debug("Still data in response buffer %zu", conn.data.response.size());
      return false;
    } else {
      debug("Finished sending response to client %d", conn.client_fd);
      conn.state = CONN_INCOMING;
      lastActivityTime.erase(conn.client_fd);
      conn.reset();
    }
  }
  return true;
}

bool settingHeadersIfNeeded(HTTPConnxData &conn) {
  if (!conn.headers_set) {
    if (!conn.data.response.empty()) {
      assert(conn.data.response.rfind("HTTP/1.1 ", 0) == 0 &&
             "Headers must start with 'HTTP/1.1 ");
      conn.data.buffer.assign(conn.data.response.begin(),
                              conn.data.response.end());
      conn.data.response.clear();
      conn.headers_set = true;
      debug("Added headers for connection %d", conn.client_fd);
    } else {
      SocketUtils::remove_from_poll(conn.client_fd);
      lastActivityTime.erase(conn.client_fd);
      conn.reset();
      return false;
    }
  }
  return true;
}

/**
 * @brief Read new data from the file if the buffer is empty
 *
 */
bool readNewDataFromFile(HTTPConnxData &conn) {
  // 1. Read new data if buffer is empty (and file not fully read)
  if (conn.data.buffer.empty() && conn.file_fd != -1) {
    char read_buf[BUFFER_SIZE];
    ssize_t bytes_read = read(conn.file_fd, read_buf, sizeof(read_buf));

    if (bytes_read < 0) {
      perror("Failed to read file");
      SocketUtils::remove_from_poll(conn.client_fd);
      lastActivityTime.erase(conn.client_fd);
      conn.reset();
      return false;
    } else if (bytes_read == 0) {
      debug("End of file reached for connection %d", conn.client_fd);
      close(conn.file_fd);
      conn.file_fd = -1;
      // keep going, there might be more data in the buffer to send to
      // client
    } else {
      // Append new data to buffer
      conn.data.buffer.insert(conn.data.buffer.end(), read_buf,
                              read_buf + bytes_read);
    }
  }
  return true;
}

bool sendNewDataFromFileToClient(HTTPConnxData &conn) {
  // 2. Send data from buffer (if any)
  if (!conn.data.buffer.empty()) {
    ssize_t bytes_sent = ::send(conn.client_fd, conn.data.buffer.data(),
                                conn.data.buffer.size(), 0);

    if (bytes_sent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        debug("Send would block, retrying later");
        return false; // Poll will retry
      }
      perror("Failed to send data");
      debuglog(RED, "Error during file transfer for connection %d",
               conn.client_fd);
      SocketUtils::remove_from_poll(conn.client_fd);
      lastActivityTime.erase(conn.client_fd);
      conn.reset();
      return false;
    } else if (bytes_sent == 0) {
      debug("No data sent to client %d", conn.client_fd);
    }
    // Remove sent bytes from buffer
    if (bytes_sent > 0) {
      conn.data.bytes_sent += static_cast<size_t>(bytes_sent);
      conn.data.buffer.erase(conn.data.buffer.begin(),
                             conn.data.buffer.begin() + bytes_sent);
      debug("Sent %zd bytes (%zu remaining in buffer)", bytes_sent,
            conn.data.buffer.size());
    }
  }
  return true;
}

/**
 * @brief Check completion conditions for file transfer
 *
 * @param conn The connection data
 *
 * This function checks if the file transfer is complete. If the file
 * descriptor is -1 and the buffer is empty, it means the file has been
 * sent completely. In this case, it resets the connection and
 * updates the state to INCOMING.
 */
void checkCompletionConditions(HTTPConnxData &conn) {
  if (conn.file_fd == -1 && conn.data.buffer.empty()) {
    debug("File sent completely for connection %d", conn.client_fd);
    debug("File transfer complete for connection %d sent %lu bytes",
          conn.client_fd, conn.data.bytes_sent);
    debuglog(YELLOW,
             "Back to state INCOMING - File transfer complete for "
             "connection %d",
             conn.client_fd);
    conn.reset();
  }
}

void uploadLoop(HTTPConnxData &conn, pollfd currentfd) {
  if (currentfd.revents & POLLIN) {
    debug("POLLIN event on upload connection %d", conn.client_fd);
    debuglog(YELLOW, "Handling upload event for connection %d",
             conn.client_fd);

    if (!readFromClientForUpload(conn) || !writeUploadToFile(conn)) {
      return;
    }
    uploadComplete(conn);
  }
}

} // namespace HTTPServer
