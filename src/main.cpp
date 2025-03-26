#include "HTTPConnection.hpp"
#include "SocketUtils.hpp"
#include "debug.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define SERVER_PORT 4244
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 4096

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
HTTPConnection connections[MAX_CONNECTIONS];
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
void init_connection(HTTPConnection *conn) {
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
  conn->is_uploading = false;
  conn->bytes_received = 0;

  // Initialize pipes
  conn->child_stdin_pipe[0] = -1;
  conn->child_stdin_pipe[1] = -1;
  conn->child_stdout_pipe[0] = -1;
  conn->child_stdout_pipe[1] = -1;
}

// Remove a connection and its associated resources
void close_connection(int conn_idx) {
  HTTPConnection *conn = &connections[conn_idx];

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

// Start a CGI process for a connection
int start_cgi_process(int conn_idx) {
  HTTPConnection *conn = &connections[conn_idx];

  // Create pipes
  if (pipe(conn->child_stdin_pipe) < 0 || pipe(conn->child_stdout_pipe) < 0) {
    perror("Failed to create pipes");
    return -1;
  }

  // Create child process
  pid_t pid = fork();

  if (pid < 0) {
    perror("Failed to fork");
    return -1;
  } else if (pid == 0) {
    // Child process

    // Close unused pipe ends
    close(conn->child_stdin_pipe[1]);  // Close write end of stdin pipe
    close(conn->child_stdout_pipe[0]); // Close read end of stdout pipe

    // Redirect stdin and stdout
    dup2(conn->child_stdin_pipe[0], STDIN_FILENO);
    dup2(conn->child_stdout_pipe[1], STDOUT_FILENO);

    // Close original file descriptors
    close(conn->child_stdin_pipe[0]);
    close(conn->child_stdout_pipe[1]);

    // Execute the Python script
    execve("cgi-bin/cgi_handler.py", NULL, NULL);

    // If execve fails
    perror("Failed to execute CGI script");
    exit(1);
  } else {
    // Parent process
    conn->poll_stdin_idx = add_to_poll(conn->child_stdin_pipe[1], POLLOUT);
    conn->poll_stdout_idx = add_to_poll(conn->child_stdout_pipe[0], POLLIN);

    // Close unused pipe ends
    close(conn->child_stdin_pipe[0]);  // Close read end of stdin pipe
    close(conn->child_stdout_pipe[1]); // Close write end of stdout pipe

    conn->child_pid = pid;
    conn->is_sending = 1;   // Ready to send data to CGI
    conn->is_receiving = 0; // Ready to receive data from CGI
    debug("Started CGI process with PID %d", pid);
    return 0;
  }
}

int main() {
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
    printf("Poll returned %d\n", poll_result);
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

        // Initialize new HTTPConnection
        HTTPConnection &conn = connections[conn_idx];
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
        HTTPConnection *conn = &connections[conn_idx];

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

          conn->data.request.append(buffer, bytes_read);

          if (!conn->parsingHeaders(conn->client_fd, *conn)) {
            close_connection(conn_idx);
            continue;
          }
          if (!conn->data.headers_received) {
            continue;
          }

          // check if the request contain cgi for debug now
          // if so set to CONN_CGI
          if (conn->data.request.find("cgi") != string::npos) {
            conn->state = CONN_CGI;
            debug("CGI request detected");
            // Start CGI process for this connection
            if (start_cgi_process(conn_idx) < 0) {
              close_connection(conn_idx);
              continue;
            }
          } else if (conn->data.request.find("upload") != string::npos) {
            conn->state = CONN_UPLOAD;
            debug("Upload request detected");
            // TODO prepare fd for upload
          } else {
            conn->state = CONN_FILE_REQUEST;
            debug("File request detected");
            // TODO prepare fd for file transfer
          }
        }

        if (conn->state == CONN_PARSING_HEADER) {
          // parse header
          // if header complete, set state
          // else continue parsing
          continue;
        }
        if (conn->state == CONN_FILE_REQUEST) {
          // check if the file is ready to be sent
          // if not set to CONN_CLOSING
          // else send the file
          continue;
        } else if (conn->state == CONN_UPLOAD) {
          // check if the upload is complete
          // if not set to CONN_CLOSING
          // else close the connection
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
			memcpy(buffer, conn->data.request.c_str(), conn->data.request.size());
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