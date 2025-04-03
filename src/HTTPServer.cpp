
#include "HTTPServer.hpp"
#include "Constants.hpp"
#include "DirectoryListing.hpp"
#include "URLMatcher.hpp"
#include "debug.h"
#include <ctime>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>
#include "SimpleResponse.hpp"

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

*/
// pollfd is an array of pollfd which contain
// the file descriptors to poll and the events we want to monitor

typedef std::vector<struct pollfd> PollfdsVector;
PollfdsVector pollfds;
vector<int> serverSockets;
map<int, HTTPConnxData> connections;
map<int, std::time_t> lastActivityTime;

// Send HTTP response headers
// maybe it should be somewhere else?
int send_headers(HTTPConnxData &conn) {
  if (!conn.data.response.empty()) {
    if (send(conn.client_fd, conn.data.response.c_str(),
             conn.data.response.size(), 0) < 0) {
      perror("Failed to send headers");
      return -1;
    }
    conn.headers_sent = true;
  }
  return 0;
}

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
      send(conn.client_fd, buffer, static_cast<size_t>(bytes_read), 0);
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

// not used? 
// void extract_filename(const char *request, char *filename) {
//   // Simple extraction - look for first line with path
//   const char *start = strstr(request, " /");
//   if (!start)
//     return;

//   start += 2; // Skip the space and slash
//   const char *end = strchr(start, ' ');
//   if (!end)
//     return;

//   size_t len = static_cast<size_t>(end - start);
//   if (len >= sizeof(connections[0].filename)) {
//     len = sizeof(connections[0].filename) - 1;
//   }

//   strncpy(filename, start, len);
//   filename[len] = '\0';
// }

int run() {
  SocketUtils::setSignalHandlers();
  Constants::initStatusMessageMap();
  Constants::initMimeTypes();
  struct sockaddr_in server_addr;
  bool skip_to_next_iteration = false;
  vector<ServerData> configs_ = Config::getServerData(NULL);
  vector<int> serverSockets;
  serverSockets.reserve(10);
  pollfds.reserve(100);

  if (configs_.empty()) {
    debuglog(RED, "No configuration data found");
    throw std::runtime_error("Error: config with empty ports");
  }
  // Create server sockets and bind to ports
  for (size_t i = 0; i < configs_.size(); i++) {
    for (size_t j = 0; j < configs_[i].ports.size(); j++) {
      int server_fd;
      if ((server_fd = SocketUtils::createBindSocket(configs_[i].ports[j])) <
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
      //   int port = configs_[i].ports[j];
      debuglog(GREEN, "Server listening on port %d", configs_[i].ports[j]);
    }
  }

  // Main polling loop
  while (1) {
    int poll_result = poll(&pollfds[0], static_cast<nfds_t>(pollfds.size()),
                           10000); // Wait indefinitely

    if (poll_result < 0) {
      if (errno != EINTR) { // Interrupted by signal
        perror("poll failed");
        break;
      }
      perror("Poll interrupted by signal");
      continue; // Interrupted by signal
    } else if (poll_result == 0) {
      continue; // Timeout
    }

    // Process events on file descriptors
    for (size_t i = 0; i < pollfds.size(); i++) {
      if (!(pollfds[i].revents & (POLLIN | POLLOUT))) {
        continue; // No events on this fd
      }

      // Exception: POLLERR, POLLHUP, and POLLNVAL can be returned even if not
      // requested
      if (pollfds[i].revents & (POLLERR | POLLNVAL)) {
        debuglog(RED, "Error condition on fd %d", pollfds[i].fd);
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(pollfds[i].fd, SOL_SOCKET, SO_ERROR, &error, &len) ==
            0) {
          debuglog(RED, "Socket error on fd %d: %s", pollfds[i].fd,
                   strerror(error));
        }
        connections[pollfds[i].fd].reset();
        SocketUtils::remove_from_poll(pollfds[i].fd);
        continue;
      }

      if (pollfds[i].revents & POLLHUP) {
        debuglog(RED, "Connection closed by client on fd %d ", pollfds[i].fd);
        connections[pollfds[i].fd].reset();
        SocketUtils::remove_from_poll(pollfds[i].fd);
        continue;
      }

      int current_fd = pollfds[i].fd;

      // Handle new connections on server socket
      for (size_t j = 0; j < serverSockets.size(); j++) {
        int server_fd = serverSockets[j];
        if (current_fd == server_fd && (pollfds[i].revents & POLLIN)) {
          struct sockaddr_in client_addr;
          socklen_t client_len = sizeof(client_addr);
          int client_fd =
              accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
          if (client_fd < 0) {
            perror("Accept failed");
			// TODO send error response
            skip_to_next_iteration = true;
            break;
          }
          // set the timeouts on the read write to client
          if (!SocketUtils::setSendRecTimeout(client_fd)) {
            perror("Failed to set send/receive timeout");
            close(client_fd);
			// TODO send error response
            skip_to_next_iteration = true;
            break;
          }
          if (pollfds.size() >= Constants::maxConnections) {
            debuglog(RED,
                     "Maximum connections reached, rejecting new connection");
            close(client_fd); // TODO
            // sendErrorResponse(clientfd, 503, "Service Unavailable");
            return false;
          }

          debug("New connection from %s:%d", inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));

          HTTPConnxData &conn = connections[client_fd];
          conn.client_fd = client_fd;
          SocketUtils::add_to_poll(client_fd, POLLIN);
          conn.state = CONN_INCOMING;

		  // Store client IP address
		  inet_ntop(AF_INET, &client_addr.sin_addr, conn.data.client_ip, sizeof(conn.data.client_ip));
		  uint16_t client_port = ntohs(client_addr.sin_port);
		  
		  // add the timeout for the client
		  lastActivityTime[client_fd] = std::time(NULL);

		  debuglog(YELLOW,"Client connected from %s:%d", conn.data.client_ip, client_port);

          debuglog(YELLOW,"Connection data initialized in state INCOMING for client %d",
                client_fd);
          skip_to_next_iteration = true;
        }
      }
	  // go back to the while loop if we had an error - TODO remove it when i get the URLMatcher func
      if (skip_to_next_iteration) {
        skip_to_next_iteration = false;
        continue;
      }

      HTTPConnxData &conn = connections[current_fd];
      if (conn.client_fd == -1) {
        debuglog(RED, "Connection fd %d not found in connections", current_fd);
        continue;
      }

      if (conn.state == CONN_INCOMING) {
        URLMatcher::validateRequest(conn);
		debuglog(YELLOW, "Connection fd %d new state %d", conn.client_fd, conn.state);
        // continue;
      }

      if (conn.state == CONN_PARSING_HEADER) {
        // parse header
        // if header complete, set state
        // else continue parsing
        continue;
      }

      if (conn.state == CONN_SIMPLE_RESPONSE) {
        debuglog(YELLOW, "Connection fd %d in state SIMPLE_RESPONSE",
                 conn.client_fd);
        // check if the response is ready to be sent
        // if not set to CONN_CLOSING
        // else send the response
        ssize_t sent = send(conn.client_fd, conn.data.response.c_str(),
                            conn.data.response.size(), 0);
        if (sent < 0) {
          perror("Failed to send simple response");
          SocketUtils::remove_from_poll(conn.client_fd);
          conn.reset();
        }
        conn.data.headers_received = false;
        conn.data = HTTPConnxData::ConnectionData();
        conn.state = CONN_INCOMING;
        SocketUtils::update_poll_events(current_fd, POLLIN);
        debuglog(YELLOW, "Switched connection %d fd back to POLLIN",
                 conn.client_fd);
        continue;
      }

      if (conn.state == CONN_FILE_REQUEST) {
        // check if the file is ready to be sent
        // if not set to CONN_CLOSING
        // else send the file
        if (pollfds[i].revents & POLLOUT) {
          debuglog(YELLOW, "Handling write event for connection fd %d",
                   conn.client_fd);

          // Use original send_headers/send_file approach
          if (!conn.headers_sent) {
            if (send_headers(conn) < 0) {
              conn.reset();
            }
          } else {
            int result = send_file(conn);
            if (result < 0) {
              conn.reset();
            } else if (result == 0) {
              // File sent completely - reset for next request
              if (conn.file_fd != -1) {
                close(conn.file_fd);
                conn.file_fd = -1;
              }
              conn.reset();

              // Switch back to POLLIN for the next request
              SocketUtils::update_poll_events(current_fd, POLLIN);
              conn.state = CONN_INCOMING;

              debuglog(YELLOW, "Switched connection %d fd back to POLLIN",
                       conn.client_fd);
            }
          }
        }
        continue;
      }

      if (conn.state == CONN_UPLOAD) {
        // check if the upload is complete
        // if not set to CONN_CLOSING
        // else close the connection
        if (pollfds[i].revents & POLLIN) {
          debuglog(YELLOW, "Handling upload event for connection %d",
                   conn.client_fd);
          // because the previous header parsing consumed data and we stored it
          if (!conn.data.response.empty()) {
            debuglog(YELLOW, "first writing content of payload");
            ssize_t bytes_written =
                write(conn.file_fd, conn.data.response.c_str(),
                      conn.data.response.size());
            if (bytes_written < 0) {
              perror("Failed to write to file");
              conn.reset();
			  // TODO send error response?
              continue;
            } else if (bytes_written == 0) {
              debug("No data written to file");
              conn.reset();
              continue;
            }
            conn.data.bytes_sent += static_cast<size_t>(bytes_written);
			conn.data.response = ""; // clear the response buffer
			if (conn.data.bytes_sent >= conn.data.content_length) {
			  debuglog(YELLOW, "Upload complete first write");
			  debuglog(YELLOW, "Written %ld bytes to %s (total: %zu)\n",
					   bytes_written, conn.filename, conn.data.bytes_sent);
			  conn.upload_completed = true;
			  close(conn.file_fd);
			  SocketUtils::update_poll_events(current_fd, POLLOUT);
			//   conn.reset();
			  lastActivityTime[current_fd] = std::time(NULL);
			  SimpleResponse::createResponse(conn, "text/plain", "File uploaded successfully.", 201);
		      conn.state = CONN_SIMPLE_RESPONSE;
			  continue;
			}
			// debuglog(YELLOW, "Not all data received yet - please continue");
			// lastActivityTime[current_fd] = std::time(NULL);
			// SimpleResponse::createResponse(conn, "text/plain", "Continue", 100);
			// conn.state = CONN_SIMPLE_RESPONSE;
			// SocketUtils::update_poll_events(current_fd, POLLOUT);
			// continue;
		  }
			// read again
			char buffer[BUFFER_SIZE];
			ssize_t bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);
			if (bytes_read <= 0) {
			  if (bytes_read == 0) {
				debug("Client disconnected");
			  } else {
				perror("recv failed");
			  }
			  conn.reset();
			  continue;
			}
			debuglog(YELLOW, "Received %ld bytes from client\n", bytes_read);
			// write to file
			ssize_t bytes_written =
				write(conn.file_fd, buffer, static_cast<size_t>(bytes_read));
			if (bytes_written < 0) {
			  perror("Failed to write to file");
			  close(conn.file_fd);
			  conn.reset();
			  continue;
			}
			conn.data.bytes_sent += static_cast<size_t>(bytes_written);
            // if we have received all data, close the connection
            if (conn.bytes_received >= conn.data.content_length) {
              debuglog(YELLOW, "Upload complete");
              debuglog(YELLOW, "Written %ld bytes to %s (total: %zu)\n",
                       bytes_written, conn.filename, conn.data.bytes_sent);
              conn.upload_completed = true;
              close(conn.file_fd);
              SocketUtils::update_poll_events(current_fd, POLLOUT);
              conn.reset();
              continue;
            } 
          }
 
      } else if (conn.state == CONN_CGI) {
        debuglog(YELLOW, "Connection fd %d in state CGI", conn.client_fd);
        // check if the cgi is ready to be sent
        // if not set to CONN_CLOSING
        // else send the cgi
        // Handle data from client and send to cgi
        if (current_fd == conn.client_fd && (pollfds[i].revents & POLLIN)) {
          char buffer[BUFFER_SIZE];
          ssize_t bytes_read = 0;
          if (conn.data.request.empty()) {
            bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
              if (bytes_read == 0) {
                debuglog(YELLOW, "Client disconnected");
              } else {
                perror("recv failed");
              }
              conn.reset();
              continue;
            }
            printf("Received %ld bytes from client\n", bytes_read);
          } else {
            memcpy(buffer, conn.data.request.c_str(), conn.data.request.size());
            bytes_read = static_cast<ssize_t>(conn.data.request.size());
          }
          // Forward data to CGI process
          ssize_t bytes_written = write(conn.child_stdin_pipe[1], buffer,
                                        static_cast<size_t>(bytes_read));
          if (bytes_written < 0) {
            perror("Write to CGI failed");
            conn.reset();
            continue;
          }
          if (bytes_read < BUFFER_SIZE) {
            // Close the write end of the pipe to signal EOF to the CGI
            debuglog(YELLOW, "Closing write end of pipe");
            close(conn.child_stdin_pipe[1]);
            conn.is_sending = 0;
            conn.is_receiving = 1;
            SocketUtils::update_poll_events(conn.child_stdin_pipe[1], 0); // Remove POLLOUT
            SocketUtils::update_poll_events(conn.child_stdout_pipe[0], POLLIN);
          }
        }
        // Handle data from CGI process (ready to write to client from cgi)
        if (conn.child_stdout_pipe[0] == current_fd &&
            (pollfds[i].revents & POLLIN)) {
          char buffer[BUFFER_SIZE];
          ssize_t bytes_read =
              read(conn.child_stdout_pipe[0], buffer, BUFFER_SIZE);
          if (bytes_read <= 0) {
            // CGI process closed pipe or error
            if (bytes_read == 0) {
              printf("CGI process finished\n");
              conn.is_receiving = 0;
            } else {
              perror("Read from CGI failed");
              conn.reset();
            }
            continue;
          }
          // Send CGI output back to client
          ssize_t bytes_sent = send(conn.client_fd, buffer, bytes_read, 0);
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
	// after every loop i check for idle connections
	SocketUtils::checkForIdleConnections();
  }
  // Cleanup
  // TODO
  return 0;
}

} // namespace HTTPServer