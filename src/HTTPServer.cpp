
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

  bool skip_to_next_iteration = false;

  SocketUtils::initialize();

  if (configs_.empty()) {
    debuglog(RED, "No configuration data found");
    throw std::runtime_error("Error: config with empty ports");
  }

  createServerSockets(configs_, serverSockets);

  while (true) {

    // when autoreload is 1 then reload the config file
    long long currentTime = Parser::getCurrentTimeMillis();
    if (Constants::autoReload) {
      if (!reload(configFile, currentTime)) {
        exit(EXIT_FAILURE);
      }
    }

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

      if (pollfds[i].revents & POLLIN && conn.state == CONN_INCOMING) {
        debug("got CONN_INCOMING fd %d", conn.client_fd);
        URLMatcher::validateRequest(conn);
        continue;
      }

      if (pollfds[i].revents & POLLIN && conn.state == CONN_PARSING_HEADER) {
        debug("CONN_PARSING_HEADER fd %d", conn.client_fd);
        // here only if the previous validate request could not parse the whole
        // headers

        debug("CONN_PARSING_HEADER fd %d", conn.client_fd);
        // parse header
        // if header complete, set state
        // else continue parsing
        continue;
      }

      // debug("CONN_SIMPLE_RESPONSE POLLOUT fd %d", conn.client_fd);
      if (pollfds[i].revents & POLLOUT && conn.state == CONN_SIMPLE_RESPONSE) {
        debug("CONN_SIMPLE_RESPONSE fd %d", conn.client_fd);
        debuglog(YELLOW, "Connection fd %d in state SIMPLE_RESPONSE",
                 conn.client_fd);
        // check if the response is ready to be sent
        // if not set to CONN_CLOSING
        // else send the response
        ssize_t sent = ::send(conn.client_fd, conn.data.response.c_str(),
                              conn.data.response.size(), 0);
        if (sent < 0) {
          perror("Failed to send simple response");
          SocketUtils::remove_from_poll(conn.client_fd);
          conn.reset();
        }
        debug("Sent response to client %d", conn.client_fd);

        conn.reset();

        continue;
      }

      // debug("CONN_FILE_REQUEST POLLOUT fd %d", conn.client_fd);

      if (pollfds[i].revents & POLLOUT && conn.state == CONN_FILE_REQUEST) {
        debug("CONN_FILE_REQUEST fd %d", conn.client_fd);
        // debuglog(YELLOW, "Handling write event for connection fd %d",
        // 	// conn.client_fd);
        // Use original send_headers/send_file approach
        if (!conn.headers_sent) {
          debug("Sending buffer headers for connection %d", conn.client_fd);
          if (send_headers(conn) < 0) {
            conn.reset();
          }
        } else {
          int result = send_file(conn);
          if (result < 0) {
            conn.reset();
            // send error?
          } else if (result == 0) {
            // File sent completely - reset for next request
            conn.reset();

            // Switch back to POLLIN for the next request
            debuglog(YELLOW, "Switched connection %d fd back to POLLIN",
                     conn.client_fd);
          }
        }
        continue;
      }

      /*
       * @brief Handle file upload
       */
      if (conn.state == CONN_UPLOAD) {
        debug("CONN_UPLOAD fd %d", conn.client_fd);
        // First handle any buffered payload data left over from header parsing
        if (!conn.data.response.empty()) {
          debuglog(
              YELLOW,
              "HTTPServer - first writing leftover payload for connection %d",
              conn.client_fd);
          ssize_t bytes_written =
              write(conn.file_fd, conn.data.response.c_str(),
                    conn.data.response.size());
          if (bytes_written <= 0) {
            perror(bytes_written < 0 ? "Failed to write to file"
                                     : "No data written to file");
            cleanup_upload(conn); // Helper to close fd and remove file
            conn.reset();
            continue;
          }

          conn.data.bytes_sent += bytes_written;
          conn.data.response.clear();

          if (conn.data.bytes_sent >= conn.data.content_length) {
            finish_upload(conn);
            continue;
          }
        }

        if (pollfds[i].revents & POLLIN) {
          debug("POLLIN event on upload connection %d", conn.client_fd);
          // Then read more data from socket
          debuglog(YELLOW,
                   "HTTPServer - Handling upload event for connection %d",
                   conn.client_fd);
          debug("read from client %d", conn.client_fd);
          char buffer[BUFFER_SIZE];
          ssize_t bytes_read =
              ::recv(conn.client_fd, buffer, sizeof(buffer), MSG_DONTWAIT);

          if (bytes_read <= 0) {
            if (bytes_read == 0) {
              debug("Client disconnected during upload");
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
              debug("No data available yet - keep in reading state");
              continue;
            }
            debug("%s", strerror(errno));
            perror("recv failed during upload");
            cleanup_upload(conn);
            conn.reset();
            continue;
          }
          debug("Received %ld bytes from client", bytes_read);
          debug("writing to file %d", conn.file_fd);
          ssize_t bytes_written = write(conn.file_fd, buffer, bytes_read);
          if (bytes_written <= 0) {
            perror(bytes_written < 0 ? "Failed to write to file"
                                     : "No data written to file");
            cleanup_upload(conn);
            conn.reset();
            continue;
          }

          conn.data.bytes_sent += bytes_written;
          debug("Wrote %ld bytes to file", bytes_written);
          debug("total bytes sent %zu", conn.data.bytes_sent);
          debug("content length %zu", conn.data.content_length);
          if (conn.data.bytes_sent >= conn.data.content_length) {
            debug("Upload complete");
            finish_upload(conn);
          }
        }
        continue;







      } else if (conn.state == CONN_CGI) {
        debug("CONN_CGI fd %d", conn.client_fd);
        debuglog(YELLOW, "Connection fd %d in state CGI", conn.client_fd);

        // check if the child process is pollin and i have data in buffer from the 
        // preparecgi function

        if (current_fd == conn.cgiData.cgi_stdin &&
            (pollfds[i].revents & POLLOUT)) {
          debug("CGI stdin fd %d", conn.client_fd);
          // Send data to CGI process
          ssize_t bytes_written =
              write(conn.cgiData.cgi_stdin, conn.cgiData.buffer.c_str(), 
                    conn.cgiData.buffer.size());
            }
        // Handle data from client and send to cgi
        if (current_fd == conn.client_fd && (pollfds[i].revents & POLLIN)) {
          char buffer[BUFFER_SIZE];
          ssize_t bytes_read = 0;
          if (conn.data.request.empty()) {
            bytes_read = ::recv(conn.client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
              if (bytes_read == 0) {
                debuglog(YELLOW, "Client disconnected");
              } else {
                perror("recv failed");
              }
              conn.reset();
              continue;
            }
            debug("Received %ld bytes from client\n", bytes_read);
          } else {
            memcpy(buffer, conn.data.request.c_str(), conn.data.request.size());
            bytes_read = static_cast<ssize_t>(conn.data.request.size());
          }
          // Forward data to CGI process
          ssize_t bytes_written =
              write(conn.cgiData.child_stdin_pipe[1], buffer,
                    static_cast<size_t>(bytes_read));
          if (bytes_written < 0) {
            perror("Write to CGI failed");
            conn.reset();
            continue;
          }
          if (bytes_read < BUFFER_SIZE) {
            // Close the write end of the pipe to signal EOF to the CGI
            debuglog(YELLOW, "Closing write end of pipe");
            close(conn.cgiData.child_stdin_pipe[1]);
            conn.cgiData.is_sending = 1;
            conn.cgiData.is_receiving = 0;
          }
        }



        // Handle data from CGI process (ready to write to client from cgi)
        if (conn.cgiData.child_stdout_pipe[0] == current_fd &&
            (pollfds[i].revents & POLLIN)) {
          char buffer[BUFFER_SIZE];
          ssize_t bytes_read =
              read(conn.cgiData.child_stdout_pipe[0], buffer, BUFFER_SIZE);
          if (bytes_read <= 0) {
            // CGI process closed pipe or error
            if (bytes_read == 0) {
              printf("CGI process finished\n");
              conn.cgiData.is_receiving = 0;
            } else {
              perror("Read from CGI failed");
              conn.reset();
            }
            continue;
          }
          // Send CGI output back to client
          ssize_t bytes_sent = ::send(conn.client_fd, buffer, bytes_read, 0);
          if (bytes_sent < 0) {
            perror("Send to client failed");
            conn.reset();
            continue;
          }

          // Close the connection after sending the response
          if (bytes_read < BUFFER_SIZE) {
            conn.reset();
          }
        }
        continue;
      }
    }
  }

  // Cleanup
  // TODO
  return 0;
}

// Send HTTP response headers
// maybe it should be somewhere else?
int send_headers(HTTPConnxData &conn) {
  if (!conn.data.response.empty()) {
    if (::send(conn.client_fd, conn.data.response.c_str(),
               conn.data.response.size(), 0) < 0) {
      perror("Failed to send headers");
      return -1;
    }
    conn.headers_sent = true;
  }
  return 0;
}

/**
 * @brief read the file in buffers and send it to the client
 *
 * @param conn the connection data
 * @return 0 if the file is sent completely, -1 on error, 1 if more data to send
 */
int send_file(HTTPConnxData &conn) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = read(conn.file_fd, buffer, sizeof(buffer));

  if (bytes_read < 0) {
    perror("Failed to read file");
    return -1;
  }
  if (bytes_read == 0) {
    close(conn.file_fd);
    conn.file_fd = -1;
    return 0; // EOF
  }

  ssize_t bytes_sent =
      ::send(conn.client_fd, buffer, static_cast<size_t>(bytes_read), 0);
  if (bytes_sent < 0) {
    perror("Failed to send data");
    return -1;
  }

  conn.data.bytes_sent += static_cast<size_t>(bytes_sent);

  // Check if we've sent the entire file
  if (conn.data.bytes_sent >= conn.file_size) {
    return 0; // File sent completely
  }

  return 1; // More data to send
}

/**
 * @brief Finish the upload process
 *
 * @param conn the connection data
 * @return void
 *
 * It closes the file descriptor and sends a 201 response to the client
 * updates the poll events to POLLOUT
 * and sets the state to CONN_SIMPLE_RESPONSE
 */
void finish_upload(HTTPConnxData &conn) {
  debug("Finishing upload for connection %d", conn.client_fd);
  debuglog(YELLOW, "Upload complete. Written %zu bytes to %s",
           conn.data.bytes_sent, conn.filename);
  close(conn.file_fd);
  conn.upload_completed = true;

  Responses::createResponse(conn, "text/plain", "File uploaded successfully.",
                            201);
  conn.state = CONN_SIMPLE_RESPONSE;
}

/**
 * @brief Cleanup the upload process when upload fails
 */
void cleanup_upload(HTTPConnxData &conn) {
  if (conn.file_fd >= 0) {
    close(conn.file_fd);
    unlink(conn.filename); // Remove partial upload
  }
}

/**
 * @brief send error and close the connection
 *
 * include the Connection: close header in the response to inform the client.
 *
 */
void send_critical_error(int fd, int code) {
  std::string response = "HTTP/1.1 " + Utils::to_string(code) + " " +
                         Constants::statusMessages[code] +
                         "\r\n"
                         "Connection: close\r\n"
                         "Content-Length: 0\r\n"
                         "\r\n";
  debug("Sending the error response %s", response.c_str());
  debug("closing the connection %d", fd);
  // i dont check for errors here because the connection will be closed
  ::send(fd, response.c_str(), response.size(), MSG_NOSIGNAL);
  ::close(fd);
  lastActivityTime.erase(fd);
  SocketUtils::remove_from_poll(fd);
  HTTPServer::connections.erase(fd);
}

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

bool reload(string configFile, long long currentTime) {

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

bool checkPollErrors(pollfd currentfd) {
  if (!(currentfd.revents & (POLLIN | POLLOUT))) {
    return true; // No events on this fd
  }

  // Exception: POLLERR, POLLHUP, and POLLNVAL can be returned even if not
  // requested
  if (currentfd.revents & POLLHUP) {
    debuglog(RED, "Connection closed by client on fd %d ", currentfd.fd);
    HTTPServer::connections[currentfd.fd].reset();
    SocketUtils::remove_from_poll(currentfd.fd);
    return true;
  }
  if (currentfd.revents & (POLLERR | POLLNVAL)) {
    debuglog(RED, "Error condition on fd %d", currentfd.fd);
    int error = 0;
    socklen_t len = sizeof(error);

    if (::getsockopt(currentfd.fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
      debug("Socket error on fd %d: %s", currentfd.fd, strerror(error));
      // Explicitly handle EPIPE (Broken pipe)
      if (error == EPIPE) {
        debug("Client disconnected (EPIPE) on fd %d", currentfd.fd);
        debuglog(YELLOW, "Client disconnected (EPIPE) on fd %d", currentfd.fd);
        // close(currentfd.fd);
      }
    }
    debug("Erasing the connection %d from the map", currentfd.fd);
    HTTPServer::connections.erase(currentfd.fd);
    debug("removing fd %d from poll", currentfd.fd);
    SocketUtils::remove_from_poll(currentfd.fd);
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
    if (!printLocalAddress(client_fd)) {
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
                                  conn.data.client_ip,
                                  sizeof(conn.data.client_ip));
    uint16_t client_port = ntohs(client_addr.sin_port);

    // add the timeout for the client
    lastActivityTime[client_fd] = std::time(NULL);

    debuglog(YELLOW, "Incoming client connected from %s:%d", conn.data.client_ip,
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
    //   NetUtils::sendErrorResponse(clientfd, 500, "Internal Server Error");
    close(clientfd);
  }

  // Set send timeout
  tv.tv_sec = Constants::responseTimeout;
  if (::setsockopt(clientfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
    debug("setsockopt SO_SNDTIMEO failed"); // todo check if this works in cgi
    //   NetUtils::sendErrorResponse(clientfd, 500, "Internal Server Error");
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
    // NetUtils::sendErrorResponse(clientfd, 503, "Service Unavailable");
    return false;
  }
  return true;
}

/**
 * @brief Print the local address of the client
 *
 * @param clientfd The client socket file descriptor
 * @return bool True if the address was printed successfully, false otherwise
 *
 * The local address refers to the IP address and port number assigned to the
 * server's socket on the local machine. This is the address that the server
 * uses to listen for incoming connections from clients.
 */
bool printLocalAddress(int clientfd) {
  struct sockaddr_in local_addr;
  socklen_t addr_len = sizeof(local_addr);
  if (::getsockname(clientfd, (struct sockaddr *)&local_addr, &addr_len) ==
      -1) {
    debug("[Server] getsockname error: %s\n", strerror(errno));
    //   NetUtils::sendErrorResponse(clientfd, 500, "Internal Server Error");
    return false;
  }
  uint16_t local_port = ntohs(local_addr.sin_port);
  debug("[Server] Accepted new connection on client socket %d, port %d",
        clientfd, local_port);
  return true;
}

HTTPConnxData &getConnectionData(int fd) {
  // First try to find as normal connection
  std::map<int, HTTPConnxData>::iterator conn_it = connections.find(fd);

  // If not found, check for CGI pipes
  if (conn_it == connections.end()) {
    for (std::map<int, HTTPConnxData>::iterator it = connections.begin();
         it != connections.end(); ++it) {
      if (it->second.cgiData.cgi_stdin == fd ||
          it->second.cgiData.cgi_stdout == fd) {
        conn_it = it;
        break;
      }
    }

    // Still not found? Throw exception
    if (conn_it == connections.end()) {
      debuglog(RED, "FD %d not found in connections", fd);
	  send_critical_error(fd, 500);
	  debug("FD %d not found in connections", fd);
      throw std::runtime_error("FD not found in connections");
    }
  }

  return conn_it->second;
}

} // namespace HTTPServer