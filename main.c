#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
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
 * curl -H "Content-Type: application/json" "http://localhost:4244/?data=This%20is%20a%20test"
 * curl -X POST -H "Content-Type: application/json" --data-raw '{"message": "This is a test"}' localhost:4244
 * curl -I -H "Content-Type: application/json" http://localhost:4244
 */
// Connection structure to store client information and pipe file descriptors
typedef struct connection_s{
    int client_fd;
	int file_fd;
	long file_size; 
	long bytes_sent;
	bool headers_sent;
} connection_t;

// Global variables to track connections and poll fds
connection_t connections[MAX_CONNECTIONS];
struct pollfd poll_fds[MAX_CONNECTIONS * 3]; // Server socket + potentially 3 fds per client (client_fd, pipe_in, pipe_out)
int poll_fd_count = 0;

// Initialize a new connection
void init_connection(int idx) {
    connections[idx].client_fd = -1;
    connections[idx].file_fd = -1;
    connections[idx].file_size = 0;
	connections[idx].bytes_sent = 0;
    connections[idx].headers_sent = false;
}

// Prepare to serve the index.html file
int prepare_response(int idx) {
	connections[idx].file_fd = open("index.html", O_RDONLY);
	debuglog(YELLOW, "Opening file index.html for fd %d", connections[idx].file_fd);
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

// Add a file descriptor to the poll array
void add_to_poll(int fd, short events) {
    poll_fds[poll_fd_count].fd = fd;
    poll_fds[poll_fd_count].events = events;
    poll_fd_count++;
}

void remove_from_poll(int fd) {
    for (int i = 0; i < poll_fd_count; i++) {
        if (poll_fds[i].fd == fd) {
            poll_fds[i] = poll_fds[poll_fd_count - 1];
            poll_fd_count--;
            break;
        }
    }
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

    connections[idx].bytes_sent += bytes_sent;
    
    // Check if we've sent the entire file
    if (connections[idx].bytes_sent >= connections[idx].file_size) {
        return 0; // File sent completely
    }
    
    return 1; // More data to send
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
			connections[i].file_fd == fd) {
            return i;
        }
    }
    return -1;
}

// Remove a connection and its associated resources
void close_connection(int idx) {
    if (connections[idx].client_fd != -1) {
        remove_from_poll(connections[idx].client_fd);
        close(connections[idx].client_fd);
        connections[idx].client_fd = -1;
    }
    if (connections[idx].file_fd != -1) {
        close(connections[idx].file_fd);
        connections[idx].file_fd = -1;
    }
    init_connection(idx);
}



int main() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        init_connection(i);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(SERVER_PORT)
    };

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

	// 5 in listen means that the server can queue up to 5 client connections before it starts rejecting them.
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", SERVER_PORT);
    add_to_poll(server_fd, POLLIN);

    while (1) {
        int ret = poll(poll_fds, poll_fd_count, 100000);
        // debuglog(YELLOW,"Poll returned %d", ret);
        if (ret < 0) {
            perror("Poll failed");
            break;
        }

        for (int i = 0; i < poll_fd_count; i++) {
            if (!poll_fds[i].revents) continue;

            int fd = poll_fds[i].fd;
            if (fd == server_fd && (poll_fds[i].revents & POLLIN)) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

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
                printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                continue;
            }

            int idx = find_connection_by_fd(fd);
            if (idx < 0) continue;

            connection_t *conn = &connections[idx];

		
            if (poll_fds[i].revents & POLLIN) {
				debuglog(YELLOW, "Handling read event for connection %d", idx);
                char buffer[BUFFER_SIZE];
                ssize_t bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_read <= 0) {
                    printf("Client disconnected\n");
                    close_connection(idx);
                    continue;
                }
                buffer[bytes_read] = '\0';
                printf("Received %ld bytes: %s\n", bytes_read, buffer);

                if (prepare_response(idx) < 0) {
					debug("Failed to prepare response");
                    close_connection(idx);
                }
				remove_from_poll(fd);
				add_to_poll(fd, POLLOUT);
				debuglog(YELLOW, "Added connection %d to poll for write", idx);
            }
			// handle write events
			 if (poll_fds[i].revents & POLLOUT) {
				debuglog(YELLOW, "Handling write event for connection %d", idx);
                if (!connections[idx].headers_sent) {
                    if (send_headers(idx) < 0) {
                        close_connection(idx);
                    }
                } else {
                    int result = send_file(idx);
                    if (result < 0) {
                        close_connection(idx);
					} else if (result == 0) {
						// File sent completely - reset for next request
						connections[idx].headers_sent = false;
						connections[idx].bytes_sent = 0;
			
						// Close the file descriptor
						if (connections[idx].file_fd != -1) {
							close(connections[idx].file_fd);
							connections[idx].file_fd = -1;
						}
			
						// Switch back to POLLIN for the next request
						remove_from_poll(fd);
						add_to_poll(fd, POLLIN);
						debuglog(YELLOW, "Switched connection %d back to POLLIN", idx);
					}
                }
            }
        }
    }

    close(server_fd);
    return 0;
}