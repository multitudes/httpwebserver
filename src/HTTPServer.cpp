#include <ctime>
#include <sys/stat.h>

#include "Constants.hpp"
#include "DirectoryListing.hpp"
#include "HTTPServer.hpp"
#include "debug.h"

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
 * telnet localhost 4244
 * curl "http://localhost:4244/?sss='this%20is'"
 * curl -H "Content-Type: application/json"
 * "http://localhost:4244/?data=This%20is%20a%20test" curl -X POST -H
 * "Content-Type: application/json" --data-raw '{"message": "This is a test"}'
 * localhost:4244 curl -I -H "Content-Type: application/json"
 * http://localhost:4244
 */

// pollfd is an array of pollfd which contain
// the file descriptors to poll and the events we want to monitor
vector<struct pollfd> pollfds;
vector<int> serverSockets;
map<int, HTTPConnxData> connections;
map<int, std::time_t> lastActivityTime;

// Global variables to track connections and poll fds

struct pollfd
    poll_fds[MAX_CONNECTIONS * 3 + 1]; // Server socket + potentially 3 fds per
                                       // client (client_fd, pipe_in, pipe_out)
int poll_fd_count = 1;

// Initialize poll_fds at startup
void init_poll_fds(int server_fd) {
  poll_fds[0].fd = server_fd;
  poll_fds[0].events = POLLIN;
  poll_fds[0].revents = 0;
  poll_fd_count = 1;
}

// Add a file descriptor to the poll array
int add_to_poll(int fd, short events) {
  if (poll_fd_count >= MAX_CONNECTIONS * 3 + 1)
    return -1;
  poll_fds[poll_fd_count].fd = fd;
  poll_fds[poll_fd_count].events = events;
  poll_fds[poll_fd_count].revents = 0;
  return poll_fd_count++;
}

// Remove a fd by swapping with last element (O(1))
void remove_from_poll(int fd) {
  for (int i = 0; i < poll_fd_count; i++) {
    if (poll_fds[i].fd == fd) {
      // Swap with last element
      if (i < poll_fd_count - 1) {
        poll_fds[i] = poll_fds[poll_fd_count - 1];
      }
      poll_fd_count--;
      break;
    }
  }
}

// Update events for an existing fd
void update_poll_events(int fd, short events) {
  for (int i = 0; i < poll_fd_count; i++) {
    if (poll_fds[i].fd == fd) {
      poll_fds[i].events = events;
      break;
    }
  }
}

// Prepare to serve the index.html file
int prepare_response(HTTPConnxData &conn) {

  DirectoryListing::getDIRListing(conn);
  if (conn.state == CONN_SIMPLE_RESPONSE) {
    return 0;
  }
  // debuglog(YELLOW, "Opening file index.html for fd %d",
  //          conn.file_fd);
  if (conn.file_fd < 0) {
    perror("Failed to open file");
    return -1;
  }

  struct stat file_stat;
  if (fstat(conn.file_fd, &file_stat) < 0) {
    perror("Failed to get file stats");
    close(conn.file_fd);
    return -1;
  }

  conn.file_size = file_stat.st_size;
  return 0;
}

// Send HTTP response headers
int send_headers(HTTPConnxData &conn) {
  char headers[512];
  int len = snprintf(headers, sizeof(headers),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: keep-alive\r\n\r\n",
                     conn.file_size);

  if (send(conn.client_fd, headers, len, 0) < 0) {
    perror("Failed to send headers");
    return -1;
  }
  conn.headers_sent = true;
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

  ssize_t bytes_sent = send(conn.client_fd, buffer, bytes_read, 0);
  if (bytes_sent < 0) {
    perror("Failed to send data");
    return -1;
  }

  conn.data.bytes_sent += bytes_sent;

  // Check if we've sent the entire file
  if (conn.data.bytes_sent >= conn.file_size) {
    return 0; // File sent completely
  }

  return 1; // More data to send
}

void extract_filename(const char *request, char *filename) {
  // Simple extraction - look for first line with path
  const char *start = strstr(request, " /");
  if (!start)
    return;

  start += 2; // Skip the space and slash
  const char *end = strchr(start, ' ');
  if (!end)
    return;

  size_t len = end - start;
  if (len >= sizeof(connections[0].filename)) {
    len = sizeof(connections[0].filename) - 1;
  }

  strncpy(filename, start, len);
  filename[len] = '\0';
}

int run() {
  // Set up signal handlers
  SocketUtils::setSignalHandlers();
  Constants::initStatusMessageMap();
  Constants::initMimeTypes();

  int server_fd;
  struct sockaddr_in server_addr;

  if ((server_fd = SocketUtils::createBindSocket(SERVER_PORT)) < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  init_poll_fds(server_fd);

  if (!SocketUtils::listenSocket(server_fd)) {
    perror("Error listening on socket");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // printf("Server listening on port %d\n", SERVER_PORT);
  debug("Server listening on port %d", SERVER_PORT);
  // Add server socket to poll
  add_to_poll(server_fd, POLLIN);

  // Main polling loop
  while (1) {

    int poll_result = poll(poll_fds, poll_fd_count, 10000); // Wait indefinitely
    // printf("Poll returned %d\n", poll_result);
    if (poll_result < 0) {
      if (errno != EINTR) { // Interrupted by signal
        perror("poll");
        break;
      }
      perror("Poll failed");
      continue; // Interrupted by signal
    } else if (poll_result == 0) {
      continue; // Timeout
    }

    // Process events on file descriptors
    for (int i = 0; i < poll_fd_count; i++) {
      if (!(poll_fds[i].revents & (POLLIN | POLLOUT))) {
        continue; // No events on this fd
      }
      if (poll_fds[i].revents & (POLLERR | POLLNVAL)) {
        debuglog(RED, "Error condition on fd %d", poll_fds[i].fd);
        connections[poll_fds[i].fd].reset();
        remove_from_poll(poll_fds[i].fd);
        continue;
      }

      if (poll_fds[i].revents & POLLHUP) {
        debuglog(RED, "Connection closed by client on fd %d ", poll_fds[i].fd);
        connections[poll_fds[i].fd].reset();
        remove_from_poll(poll_fds[i].fd);
        continue;
      }

      int current_fd = poll_fds[i].fd;

      // Handle new connections on server socket
      if (current_fd == server_fd && (poll_fds[i].revents & POLLIN)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
          perror("Accept failed");
          continue;
        }

        debug("New connection from %s:%d", inet_ntoa(client_addr.sin_addr),
              ntohs(client_addr.sin_port));

        HTTPConnxData &conn = connections[client_fd];
        conn.client_fd = client_fd;
        conn.poll_client_idx = add_to_poll(client_fd, POLLIN);
        conn.state = CONN_INCOMING;

        // Set client info in connection data
        conn.data.host = inet_ntoa(client_addr.sin_addr);
        debug("Connection data initialized in state INCOMING for client %d",
              client_fd);
        continue;
      }

      if (connections.find(current_fd) == connections.end()) {
        debuglog(RED, "Connection %d not found in connections", current_fd);
        remove_from_poll(current_fd); // Remove fd from poll array
        continue;
      }

      HTTPConnxData &conn = connections[current_fd];
      if (conn.client_fd == -1) {
        debuglog(RED, "Connection fd %d not found in connections", current_fd);
        continue;
      }

      if (conn.state == CONN_INCOMING) {
        debug("connection incoming");
        debuglog(YELLOW, "Connection on fd %d in state INCOMING",
                 conn.client_fd);
        // check if the headers are received
        // if not set to CONN_PARSING_HEADER
        char buffer[BUFFER_SIZE];
        int bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);

        if (bytes_read <= 0) {
          if (bytes_read == 0) {
            debug("Client disconnected");
          } else {
            perror("recv failed");
          }
          conn.reset();
          continue;
        }

        printf("Received %d bytes from client\n", bytes_read);
        // debug("request: %s", buffer);
        conn.data.request.append(buffer, bytes_read);

        if (!conn.parsingHeaders(conn.client_fd, conn)) {
          remove_from_poll(conn.client_fd);
          conn.reset();
          continue;
        }
        // debugcolor(PASTEL_MAGENTA,"Headers received \n%s",
        // conn.data.request.c_str());
        if (!conn.data.headers_received) {
          continue;
        }

        // check if the request contain cgi for debug now
        // if so set to CONN_CGI
        if (conn.data.request.find("cgi") != string::npos) {
          conn.state = CONN_CGI;
          debug("CGI request detected");
          // Start CGI process for this connection
          if (CGI::prepareCGI(conn) < 0) {
            conn.reset();
            continue;
          }
        } else if (conn.data.request.find("upload") != string::npos) {
          conn.state = CONN_UPLOAD;
          debuglog(YELLOW, "Upload request detected");
          // TODO prepare fd for upload

          // First chunk - extract filename and open file
          extract_filename(buffer, conn.filename);

          conn.file_fd =
              open(conn.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (conn.file_fd < 0) {
            perror("Failed to open file for upload");
            conn.reset();
            continue;
          }

          debuglog(YELLOW, "Starting upload to: %s\n", conn.filename);

        } else {
          conn.state = CONN_FILE_REQUEST;
          debuglog(YELLOW, "File request detected");
          // TODO prepare fd for file transfer
          if (prepare_response(conn) < 0) {
            debug("Failed to prepare response");
            conn.reset();
          }
          update_poll_events(current_fd, POLLOUT);
          debuglog(YELLOW, "Sending response to fd %d", conn.client_fd);
        }
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
          remove_from_poll(conn.client_fd);
          conn.reset();
        }
        conn.data.headers_received = false;
        conn.data = HTTPConnxData::ConnectionData();
        conn.state = CONN_INCOMING;
		update_poll_events(current_fd, POLLIN);
        continue;
      }
      if (conn.state == CONN_FILE_REQUEST) {
        // check if the file is ready to be sent
        // if not set to CONN_CLOSING
        // else send the file
        if (poll_fds[i].revents & POLLOUT) {
          debuglog(YELLOW, "Handling write event for connection fd %d",
                   conn.client_fd);
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
              // Close the file descriptor
              if (conn.file_fd != -1) {
                close(conn.file_fd);
              }
              conn.reset();

              // Switch back to POLLIN for the next request
              update_poll_events(current_fd, POLLIN);
              conn.state = CONN_INCOMING;

              debuglog(YELLOW, "Switched connection %d fd back to POLLIN",
                       conn.client_fd);
            }
          }
        }

        continue;
      } else if (conn.state == CONN_UPLOAD) {
        // check if the upload is complete
        // if not set to CONN_CLOSING
        // else close the connection
        if (poll_fds[i].revents & POLLIN) {
          debuglog(YELLOW, "Handling upload event for connection %d",
                   conn.client_fd);
          // because the previous header parsing consumed data and we stored it
          if (!conn.data.request.empty()) {
            debuglog(YELLOW, "first writing content of request");
            ssize_t bytes_written =
                write(conn.file_fd, conn.data.request.c_str(),
                      conn.data.request.size());
            if (bytes_written < 0) {
              perror("Failed to write to file");
              conn.reset();
              continue;
            } else if (bytes_written == 0) {
              debug("No data written to file");
              conn.reset();
              continue;
            }
            conn.bytes_received += bytes_written;
            // if we have received all data, close the connection
            if (conn.bytes_received >= conn.data.content_length) {
              debuglog(YELLOW, "Upload complete first write");
              debuglog(YELLOW, "Written %ld bytes to %s (total: %zu)\n",
                       bytes_written, conn.filename, conn.bytes_received);
              conn.upload_completed = true;
              close(conn.file_fd);
              update_poll_events(current_fd, POLLOUT);
              conn.reset();
            } else {
              continue;
            }
          } else {
            // read again
            char buffer[BUFFER_SIZE];
            int bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
              if (bytes_read == 0) {
                debug("Client disconnected");
              } else {
                perror("recv failed");
              }
              conn.reset();
              continue;
            }
            printf("Received %d bytes from client\n", bytes_read);
            //   conn.data.request.append(buffer, bytes_read);
            debuglog(YELLOW, "Received %d bytes from client\n", bytes_read);
            // erite to file
            ssize_t bytes_written = write(conn.file_fd, buffer, bytes_read);
            if (bytes_written < 0) {
              perror("Failed to write to file");
              close(conn.file_fd);
              conn.reset();
              continue;
            }
            conn.bytes_received += bytes_written;
            // if we have received all data, close the connection
            if (conn.bytes_received >= conn.data.content_length) {
              debuglog(YELLOW, "Upload complete");
              debuglog(YELLOW, "Written %ld bytes to %s (total: %zu)\n",
                       bytes_written, conn.filename, conn.bytes_received);
              conn.upload_completed = true;
              update_poll_events(current_fd, POLLOUT);
            } else {
              continue;
            }
          }
          debuglog(YELLOW, "here here here? ");
          // Handle client ready for write (response)
          if (poll_fds[i].revents & POLLOUT && conn.upload_completed) {
            debuglog(YELLOW, "Handling upload response for connection fd %d",
                     conn.client_fd);
            const char *response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Length: 18\r\n"
                                   "Connection: close\r\n\r\n"
                                   "Upload successful\n";

            if (send(conn.client_fd, response, strlen(response), 0) < 0) {
              perror("Failed to send response");
            }
            update_poll_events(current_fd, POLLIN);

            conn.reset();
            debuglog(YELLOW, "Upload response sent to client");
          }
        }
        continue;
      } else if (conn.state == CONN_CGI) {
        debuglog(YELLOW, "Connection fd %d in state CGI", conn.client_fd);
        // check if the cgi is ready to be sent
        // if not set to CONN_CLOSING
        // else send the cgi
        // Handle data from client and send to cgi
        if (current_fd == conn.client_fd && (poll_fds[i].revents & POLLIN)) {
          char buffer[BUFFER_SIZE];
          int bytes_read = 0;
          if (conn.data.request.empty()) {

            bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);

            if (bytes_read <= 0) {
              if (bytes_read == 0) {
                // printf("Client disconnected\n");
                debuglog(YELLOW, "Client disconnected");
              } else {
                perror("recv failed");
              }
              conn.reset();
              continue;
            }

            printf("Received %d bytes from client\n", bytes_read);

          } else {
            bytes_read = conn.data.request.size();
            memcpy(buffer, conn.data.request.c_str(), conn.data.request.size());
          }
          // Forward data to CGI process

          int bytes_written =
              write(conn.child_stdin_pipe[1], buffer, bytes_read);
          if (bytes_written < 0) {
            perror("Write to CGI failed");
            conn.reset();
            continue;
          }

          if (bytes_read < BUFFER_SIZE) {
            // Close the write end of the pipe to signal EOF to the CGI
            // process
            debuglog(YELLOW, "Closing write end of pipe");
            close(conn.child_stdin_pipe[1]);
            conn.is_sending = 0;
            conn.is_receiving = 1;
            update_poll_events(conn.child_stdin_pipe[1],
                               0); // Remove POLLOUT
            update_poll_events(conn.child_stdout_pipe[0], POLLIN);
          }
        }

        // Handle data from CGI process (ready to write to client from cgi)
        if (conn.child_stdout_pipe[0] == current_fd &&
            (poll_fds[i].revents & POLLIN)) {
          char buffer[BUFFER_SIZE];
          int bytes_read = read(conn.child_stdout_pipe[0], buffer, BUFFER_SIZE);

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
          // printf("Received %d bytes from cgi\n", bytes_read);

          // Send CGI output back to client
          int bytes_sent = send(conn.client_fd, buffer, bytes_read, 0);
          if (bytes_sent < 0) {
            perror("Send to client failed");
            conn.reset();
            continue;
          }

          // printf("Sent %d bytes to client\n", bytes_sent);

          // Close the connection after sending the response
          if (bytes_read < BUFFER_SIZE) {
            // printf("Closing client connection\n");
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

} // namespace HTTPServer