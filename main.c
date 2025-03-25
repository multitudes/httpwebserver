#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include "debug.h"


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
 * 
 * this will create multipart file upload
 * curl -F "file=@test.txt" http://localhost:4244/test.txt
 * 
 * to test the file upload
 * curl --data-binary @test.txt http://localhost:4244/test.txt
 */
// Connection structure to store client information and pipe file descriptors
typedef struct connection_s {
  int client_fd;
  int file_fd;
  char filename[256]; // Store filename
  bool is_uploading;
} connection_t;

// Global variables to track connections and poll fds
connection_t connections[MAX_CONNECTIONS];
struct pollfd
    poll_fds[MAX_CONNECTIONS * 3]; // Server socket + potentially 3 fds per
                                   // client (client_fd, pipe_in, pipe_out)
int poll_fd_count = 0;

// Add a file descriptor to the poll array
void add_to_poll(int fd, short events) {
  poll_fds[poll_fd_count].fd = fd;
  poll_fds[poll_fd_count].events = events;
  poll_fd_count++;
}

// Initialize a new connection
void init_connection(connection_t *conn) {
	conn->client_fd = -1;
	conn->file_fd = -1;
	memset(conn->filename, 0, sizeof(conn->filename));
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
    if (connections[i].client_fd == fd || connections[i].file_fd == fd) {
      return i;
    }
  }
  return -1;
}

// Remove a connection and its associated resources
void close_connection(int conn_idx) {
  connection_t *conn = &connections[conn_idx];

  if (conn->client_fd != -1) close(conn->client_fd);
  if (conn->file_fd != -1) close(conn->file_fd);
  memset(conn, 0, sizeof(connection_t));
  conn->client_fd = -1;
  conn->file_fd = -1;

}

// Update the poll_fds array (rebuild it from scratch)
void update_poll_fds(int server_fd) {
  poll_fd_count = 0;

  // Add server socket first (only need to check for incoming connections)
  add_to_poll(server_fd, POLLIN);

  // Add client connections and pipes
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    connection_t *conn = &connections[i];

    if (conn->client_fd != -1) {
      // Add client socket fd - monitor for both read and write
      add_to_poll(conn->client_fd, POLLIN | POLLOUT);
      // if the file_fd is open, add it to the poll list.
      if (conn->file_fd != -1) {
        add_to_poll(conn->file_fd, POLLIN);
      }
    }
  }
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    init_connection(&connections[i]);
  }

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // 5 in listen means that the server can queue up to 5 client connections
  // before it starts rejecting them.
  if (listen(server_fd, 5) < 0) {
    perror("Listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server listening on port %d\n", SERVER_PORT);
  add_to_poll(server_fd, POLLIN);

  while (1) {
    int ret = poll(poll_fds, poll_fd_count, -1);
    if (ret < 0) {
      perror("Poll failed");
      break;
    }

    for (int i = 0; i < poll_fd_count; i++) {
      if (!poll_fds[i].revents)
        continue;

      int fd = poll_fds[i].fd;
      if (fd == server_fd && (poll_fds[i].revents & POLLIN)) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
          perror("Accept failed");
          continue;
        }

        int idx = find_free_connection();
        if (idx < 0) {
          printf("No free slots\n");
          close(client_fd);
          continue;
        }

        connections[idx].client_fd = client_fd;
        add_to_poll(client_fd, POLLIN);
        printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        continue;
      }

      int idx = find_connection_by_fd(fd);
      if (idx < 0)
        continue;

      connection_t *conn = &connections[idx];

	  if (fd == conn->client_fd && (poll_fds[i].revents & POLLIN)) {
		char buffer[BUFFER_SIZE];
		ssize_t bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE, 0);

		if (bytes_read <= 0) {
			printf("Client disconnected\n");
			close_connection(idx);
			update_poll_fds(server_fd);
			continue;
		}
		printf("Received %ld bytes\nreq is\n%s", bytes_read, buffer);

		if (!conn->is_uploading) {
			// Simple filename extraction
			strncpy(conn->filename, buffer, sizeof(conn->filename) - 1);
			conn->filename[sizeof(conn->filename) - 1] = '\0';
			conn->file_fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (conn->file_fd != -1) {
				conn->is_uploading = true;
				update_poll_fds(server_fd);
				printf("File opened successfully for writing: test.txt\n");
			} else {
				perror("Failed to open file");
				close_connection(idx);
				update_poll_fds(server_fd);
			}
			// this will not happen because the whole req 
			// is already consumed by the recv call previous to this.
			// but folr bigger uploads it will get thet data in parts
			// with curl -F option is sending multipart data...
			// we can chose not to handle it? 
			// or it sends raw data in parts? more logic is needed
		} 

		// here if the file is already opened and we are getting more of the file
		if (conn->is_uploading) {
			write(conn->file_fd, buffer, bytes_read);
			if (bytes_read < BUFFER_SIZE) {
				printf("Upload complete\n");
				// close_connection(idx);
				// update_poll_fds(server_fd);
			}
		}
	} else if (fd == conn->client_fd && (poll_fds[i].revents & POLLOUT)) {
		
			char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 18\r\nConnection: close\r\n\r\nUpload successful\n";
			send(conn->client_fd, response, strlen(response), 0);
			close_connection(idx);
			update_poll_fds(server_fd);
		
	}
    }
  }

  close(server_fd);
  return 0;
}