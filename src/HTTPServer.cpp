#include "HTTPServer.hpp"
#include "debug.h"
#include <sys/stat.h>
#include "DirectoryListing.hpp"

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

// Global variables to track connections and poll fds
HTTPConnxData connections[MAX_CONNECTIONS];
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

// Find an available connection slot
int find_free_connection() {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (connections[i].client_fd == -1) {
      return i;
    }
  }
  return -1; // No free slots
}

// Find the connection index for a given file descriptor
int find_connection_by_fd(int fd) {
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (connections[i].client_fd == fd ||
        connections[i].child_stdin_pipe[1] == fd ||
        connections[i].child_stdout_pipe[0] == fd) {
      return i;
    }
  }
  return -1;
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

// Initialize a connection
void init_connection(HTTPConnxData *conn) {
  conn->client_fd = -1;
  conn->indexServerConf = -1;
  conn->poll_client_idx = -1;
  conn->is_sending = 0;
  conn->is_receiving = 0;
  conn->headers_sent = false;
  conn->cgi_processing = false;
  conn->file_fd = -1;
  conn->file_size = 0;
  conn->file_offset = 0;
  conn->writeto_fd = -1;
  conn->bytes_received = 0;

  // Initialize pipes
  conn->child_stdin_pipe[0] = -1;
  conn->child_stdin_pipe[1] = -1;
  conn->child_stdout_pipe[0] = -1;
  conn->child_stdout_pipe[1] = -1;
}

// Remove a connection and its associated resources
void close_connection(int conn_idx) {
  HTTPConnxData *conn = &connections[conn_idx];

  if (conn->client_fd != -1) {
    close(conn->client_fd);
    remove_from_poll(conn->client_fd);
  }

  if (conn->child_stdin_pipe[0] != -1) {
    close(conn->child_stdin_pipe[0]);
    remove_from_poll(conn->child_stdin_pipe[0]);
  }
  if (conn->child_stdin_pipe[1] != -1) {
    close(conn->child_stdin_pipe[1]);
    remove_from_poll(conn->child_stdin_pipe[1]);
  }
  if (conn->child_stdout_pipe[0] != -1) {
    close(conn->child_stdout_pipe[0]);
    remove_from_poll(conn->child_stdout_pipe[0]);
  }
  if (conn->child_stdout_pipe[1] != -1) {
    close(conn->child_stdout_pipe[1]);
    remove_from_poll(conn->child_stdout_pipe[1]);
  }
  // Kill child process if it's still running
  if (conn->child_pid > 0) {
    kill(conn->child_pid, SIGTERM);
  }

  // Reset the connection
  conn->reset(); // Clean any previous state
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
int prepare_response(int idx) {
  // connections[idx].file_fd = open("index.html", O_RDONLY);
  DirectoryListing::getDIRListing(connections[idx]);
  if (connections[idx].state == CONN_SIMPLE_RESPONSE) {
    return 0;
  }
  //debuglog(YELLOW, "Opening file index.html for fd %d",
  //         connections[idx].file_fd);
  if (connections[idx].file_fd < 0) {
    perror("Failed to open file");
    return -1;
  }

  struct stat file_stat;
  if (fstat(connections[idx].file_fd, &file_stat) < 0) {
    perror("Failed to get file stats");
    close(connections[idx].file_fd);
    return -1;
  }

  connections[idx].file_size = file_stat.st_size;
  return 0;
}

// Send HTTP response headers
int send_headers(int idx) {
  char headers[512];
  int len = snprintf(headers, sizeof(headers),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: keep-alive\r\n\r\n",
                     connections[idx].file_size);

  if (send(connections[idx].client_fd, headers, len, 0) < 0) {
    perror("Failed to send headers");
    return -1;
  }
  connections[idx].headers_sent = true;
  return 0;
}

int send_file(int idx) {
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read = read(connections[idx].file_fd, buffer, sizeof(buffer));

  if (bytes_read < 0) {
    perror("Failed to read file");
    return -1;
  }
  if (bytes_read == 0) {
    close(connections[idx].file_fd);
    connections[idx].file_fd = -1;
    return 0; // EOF
  }

  ssize_t bytes_sent = send(connections[idx].client_fd, buffer, bytes_read, 0);
  if (bytes_sent < 0) {
    perror("Failed to send data");
    return -1;
  }

  connections[idx].data.bytes_sent += bytes_sent;

  // Check if we've sent the entire file
  if (connections[idx].data.bytes_sent >= connections[idx].file_size) {
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
        // cleanup_connection(pollfds, client_to_file, i);

        continue;
      }

      if (poll_fds[i].revents & POLLHUP) {
        debuglog(RED, "Connection closed by client on fd %d ", poll_fds[i].fd);
        // cleanup_connection(pollfds, client_to_file, i);
        int conn_idx = find_connection_by_fd(poll_fds[i].fd);
        if (conn_idx >= 0) {
          close_connection(conn_idx);
        }
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

        int conn_idx = find_free_connection();
        if (conn_idx < 0) {
          debug("No free connection slots");
          close(client_fd);
          continue;
        }

        // Initialize new HTTPConnxData
        HTTPConnxData &conn = connections[conn_idx];
        conn.reset(); // Clean any previous state

        // Set basic connection info
        conn.client_fd = client_fd;
        conn.poll_client_idx = add_to_poll(client_fd, POLLIN);
        conn.state = CONN_INCOMING;

        // Set client info in connection data
        conn.data.host = inet_ntoa(client_addr.sin_addr);
        debug("Connection %d initialized in state INCOMING", conn_idx);
        continue;
      }

      // Handle client data
      int conn_idx = find_connection_by_fd(current_fd);
      if (conn_idx >= 0) {
        HTTPConnxData *conn = &connections[conn_idx];

        if (conn->state == CONN_INCOMING) {
          debuglog(YELLOW, "Connection %d in state INCOMING", conn_idx);
          // check if the headers are received
          // if not set to CONN_PARSING_HEADER
          char buffer[BUFFER_SIZE];
          int bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE, 0);

          if (bytes_read <= 0) {
            if (bytes_read == 0) {
              debug("Client disconnected");
            } else {
              perror("recv failed");
            }
            close_connection(conn_idx);
            continue;
          }

          printf("Received %d bytes from client\n", bytes_read);
          // debug("request: %s", buffer);
          conn->data.request.append(buffer, bytes_read);

          if (!conn->parsingHeaders(conn->client_fd, *conn)) {
            close_connection(conn_idx);
            continue;
          }
         // debugcolor(PASTEL_MAGENTA,"Headers received \n%s", conn->data.request.c_str());
          if (!conn->data.headers_received) {
            continue;
          }

          // check if the request contain cgi for debug now
          // if so set to CONN_CGI
          if (conn->data.request.find("cgi") != string::npos) {
            conn->state = CONN_CGI;
            debug("CGI request detected");
            // Start CGI process for this connection
            if (CGI::prepareCGI(conn) < 0) {
              close_connection(conn_idx);
              continue;
            }
          } else if (conn->data.request.find("upload") != string::npos) {
            conn->state = CONN_UPLOAD;
            debuglog(YELLOW, "Upload request detected");
            // TODO prepare fd for upload

            // First chunk - extract filename and open file
            extract_filename(buffer, conn->filename);

            conn->file_fd =
                open(conn->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (conn->file_fd < 0) {
              perror("Failed to open file for upload");
              close_connection(conn_idx);
              continue;
            }

            debuglog(YELLOW, "Starting upload to: %s\n", conn->filename);

          } else {
            conn->state = CONN_FILE_REQUEST;
            debuglog(YELLOW, "File request detected");
            // TODO prepare fd for file transfer
            if (prepare_response(conn_idx) < 0) {
              debug("Failed to prepare response");
              close_connection(conn_idx);
            }
            update_poll_events(current_fd, POLLOUT);
            debuglog(YELLOW, "Sending response to connx %d", conn_idx);
          }
        }

        if (conn->state == CONN_PARSING_HEADER) {
          // parse header
          // if header complete, set state
          // else continue parsing
          continue;
        }
        if (conn->state == CONN_SIMPLE_RESPONSE) {
          
          debuglog(YELLOW, "Connection %d in state SIMPLE_RESPONSE", conn_idx);
          // check if the response is ready to be sent
          // if not set to CONN_CLOSING
          // else send the response
          send(conn->client_fd, conn->data.response.c_str(),
               conn->data.response.size(), 0);
          // close_connection(conn_idx);
          init_connection(conn);

          conn->state = CONN_INCOMING;
          continue;


        }
        if (conn->state == CONN_FILE_REQUEST) {
          // check if the file is ready to be sent
          // if not set to CONN_CLOSING
          // else send the file
          if (poll_fds[i].revents & POLLOUT) {
            debuglog(YELLOW, "Handling write event for connection %d",
                     conn_idx);
            if (!connections[conn_idx].headers_sent) {
              if (send_headers(conn_idx) < 0) {
                close_connection(conn_idx);
              }
            } else {
              int result = send_file(conn_idx);
              if (result < 0) {
                close_connection(conn_idx);
              } else if (result == 0) {
                // File sent completely - reset for next request
                connections[conn_idx].headers_sent = false;
                connections[conn_idx].data.bytes_sent = 0;

                // Close the file descriptor
                if (connections[conn_idx].file_fd != -1) {
                  close(connections[conn_idx].file_fd);
                  connections[conn_idx].file_fd = -1;
                }

                // Switch back to POLLIN for the next request
                remove_from_poll(current_fd);
                add_to_poll(current_fd, POLLIN);
                connections[conn_idx].state = CONN_INCOMING;
                connections[conn_idx].data.request.clear();
                close(connections[conn_idx].file_fd);
                debuglog(YELLOW, "Switched connection %d back to POLLIN",
                         conn_idx);
              }
            }
          }

          continue;
        } else if (conn->state == CONN_UPLOAD) {
          // check if the upload is complete
          // if not set to CONN_CLOSING
          // else close the connection
          if (poll_fds[i].revents & POLLIN) {
            debuglog(YELLOW, "Handling upload event for connection %d",
                     conn_idx);
			// because the previous header parsing consumed data and we stored it
            if (!conn->data.request.empty()) {
				debuglog(YELLOW, "first writing content of request");
              ssize_t bytes_written =
                  write(conn->file_fd, conn->data.request.c_str(),
                        conn->data.request.size());
              if (bytes_written < 0) {
                perror("Failed to write to file");
                close_connection(conn_idx);
                continue;
              } else if (bytes_written == 0) {
				debug("No data written to file");
				close_connection(conn_idx);
				conn->state = CONN_INCOMING;
				continue;
			  }
              conn->bytes_received += bytes_written;
			  // if we have received all data, close the connection
			  if (conn->bytes_received >= conn->data.content_length) {
				debuglog(YELLOW,"Upload complete first write");
				debuglog(YELLOW, "Written %ld bytes to %s (total: %zu)\n",
						 bytes_written, conn->filename, conn->bytes_received);
				conn->upload_completed = true;
				close(conn->file_fd);
				conn->file_fd = -1;
				conn->data.request.clear();
				update_poll_events(current_fd, POLLOUT);
			  } else {
				continue;
			  }
            } else {
              // read again
              char buffer[BUFFER_SIZE];
              int bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE, 0);
              if (bytes_read <= 0) {
				  if (bytes_read == 0) {
					  debug("Client disconnected");
					} else {
						perror("recv failed");
					}
				conn->state = CONN_INCOMING;
				close_connection(conn_idx);
                continue;
              }
              printf("Received %d bytes from client\n", bytes_read);
              //   conn->data.request.append(buffer, bytes_read);
              debuglog(YELLOW, "Received %d bytes from client\n", bytes_read);
              // erite to file
              ssize_t bytes_written = write(conn->file_fd, buffer, bytes_read);
              if (bytes_written < 0) {
                perror("Failed to write to file");
				conn->upload_completed = true;
				close(conn->file_fd);
				conn->state = CONN_INCOMING;
                close_connection(conn_idx);
                continue;
              }
              conn->bytes_received += bytes_written;
			  // if we have received all data, close the connection
			  if (conn->bytes_received >= conn->data.content_length) {
				debuglog(YELLOW,"Upload complete");
				debuglog(YELLOW, "Written %ld bytes to %s (total: %zu)\n",
				bytes_written, conn->filename, conn->bytes_received);
				conn->upload_completed = true;
				update_poll_events(current_fd, POLLOUT);
			  } else {
				continue;
			  }
            }
			debuglog(YELLOW, "here here here? ");
            // Handle client ready for write (response)
            if (poll_fds[i].revents & POLLOUT && conn->upload_completed) {
				debuglog(YELLOW, "Handling upload response for connection %d",
						 conn_idx);
              const char *response = "HTTP/1.1 200 OK\r\n"
                                     "Content-Length: 18\r\n"
                                     "Connection: close\r\n\r\n"
                                     "Upload successful\n";

              if (send(conn->client_fd, response, strlen(response), 0) < 0) {
                perror("Failed to send response");
              }
			  update_poll_events(current_fd, POLLIN);
			  //   close_connection(conn_idx);
			//   conn->state = CONN_INCOMING;
			//   conn->upload_completed = false;
				close_connection(conn_idx);
			  debuglog(YELLOW, "Upload response sent to client");
			  // Reset connection state
			//   conn->bytes_received = 0;
            }
          }
          continue;
        } else if (conn->state == CONN_CGI) {
          debuglog(YELLOW, "Connection %d in state CGI", conn_idx);
          // check if the cgi is ready to be sent
          // if not set to CONN_CLOSING
          // else send the cgi
          // Handle data from client and send to cgi
          if (current_fd == conn->client_fd && (poll_fds[i].revents & POLLIN)) {
            char buffer[BUFFER_SIZE];
            int bytes_read = 0;
            if (conn->data.request.empty()) {

              bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE, 0);

              if (bytes_read <= 0) {
                if (bytes_read == 0) {
                  // printf("Client disconnected\n");
                } else {
                  perror("recv failed");
                }
                close_connection(conn_idx);
                continue;
              }

              printf("Received %d bytes from client\n", bytes_read);

            } else {
              bytes_read = conn->data.request.size();
              memcpy(buffer, conn->data.request.c_str(),
                     conn->data.request.size());
            }
            // Forward data to CGI process

            int bytes_written =
                write(conn->child_stdin_pipe[1], buffer, bytes_read);
            if (bytes_written < 0) {
              perror("Write to CGI failed");
              close_connection(conn_idx);
              continue;
            }

            if (bytes_read < BUFFER_SIZE) {
              // Close the write end of the pipe to signal EOF to the CGI
              // process
              printf("Closing write end of pipe\n");
              close(conn->child_stdin_pipe[1]);
              conn->is_sending = 0;
              conn->is_receiving = 1;
              update_poll_events(conn->child_stdin_pipe[1],
                                 0); // Remove POLLOUT
              update_poll_events(conn->child_stdout_pipe[0], POLLIN);
            }
          }

          // Handle data from CGI process (ready to write to client from cgi)
          if (conn->child_stdout_pipe[0] == current_fd &&
              (poll_fds[i].revents & POLLIN)) {
            char buffer[BUFFER_SIZE];
            int bytes_read =
                read(conn->child_stdout_pipe[0], buffer, BUFFER_SIZE);

            if (bytes_read <= 0) {
              // CGI process closed pipe or error
              if (bytes_read == 0) {
                printf("CGI process finished\n");
                conn->is_receiving = 0;
              } else {
                perror("Read from CGI failed");
                close_connection(conn_idx);
              }
              continue;
            }
            // printf("Received %d bytes from cgi\n", bytes_read);

            // Send CGI output back to client
            int bytes_sent = send(conn->client_fd, buffer, bytes_read, 0);
            if (bytes_sent < 0) {
              perror("Send to client failed");
              close_connection(conn_idx);
              continue;
            }

            // printf("Sent %d bytes to client\n", bytes_sent);

            // Close the connection after sending the response
            if (bytes_read < BUFFER_SIZE) {
              // printf("Closing client connection\n");
              close_connection(conn_idx);
            }
          }

          continue;
        }
      }
    }
  }

  // Cleanup
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (connections[i].client_fd != -1) {
      close_connection(i);
    }
  }

  close(server_fd);
  return 0;
}

} // namespace HTTPServer