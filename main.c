#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
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

typedef struct connection_s {
    int client_fd;
    int file_fd;
    char filename[256];
    bool is_uploading;
    size_t bytes_received;
} connection_t;

connection_t connections[MAX_CONNECTIONS];
struct pollfd poll_fds[MAX_CONNECTIONS + 1]; // Server + clients
int poll_fd_count = 0;

void init_connection(int idx) {
	debuglog(YELLOW,"Initializing connection %d", idx);
    connections[idx].client_fd = -1;
    connections[idx].file_fd = -1;
    memset(connections[idx].filename, 0, sizeof(connections[idx].filename));
    connections[idx].is_uploading = false;
    connections[idx].bytes_received = 0;
}

void add_to_poll(int fd, short events) {
    if (poll_fd_count >= MAX_CONNECTIONS + 1) return;
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

int find_free_connection() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].client_fd == -1) {
            return i;
        }
    }
    return -1;
}

void close_connection(int idx) {
    if (connections[idx].client_fd != -1) {
        remove_from_poll(connections[idx].client_fd);
        close(connections[idx].client_fd);
    }
    if (connections[idx].file_fd != -1) {
        close(connections[idx].file_fd);
    }
    init_connection(idx);
}

void extract_filename(const char *request, char *filename) {
    // Simple extraction - look for first line with path
    const char *start = strstr(request, " /");
    if (!start) return;
    
    start += 2; // Skip the space and slash
    const char *end = strchr(start, ' ');
    if (!end) return;
    
    size_t len = end - start;
    if (len >= sizeof(connections[0].filename)) {
        len = sizeof(connections[0].filename) - 1;
    }
    
    strncpy(filename, start, len);
    filename[len] = '\0';
}

int main() {
    // Initialize connections
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        init_connection(i);
    }

    // Create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
	debuglog(YELLOW,"Socket created with fd %d", server_fd);

    // Set socket options
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	debuglog(YELLOW,"Socket options set to non blocking");
    // Bind socket
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(SERVER_PORT)
    };

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
	debuglog(YELLOW,"Socket bound to port %d", SERVER_PORT);

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
	debuglog(YELLOW,"Socket listening on port %d", SERVER_PORT);

    add_to_poll(server_fd, POLLIN);

    while (1) {
        int ret = poll(poll_fds, poll_fd_count, -1);
        if (ret < 0) {
            perror("Poll failed");
            break;
        }

        for (int i = 0; i < poll_fd_count; i++) {
            if (!poll_fds[i].revents) continue;

            int fd = poll_fds[i].fd;
            
            // Handle new connections
            if (fd == server_fd) {
				debuglog(YELLOW,"New connection on server socket");
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

                if (client_fd < 0) {
                    perror("Accept failed");
                    continue;
                }

                int idx = find_free_connection();
                if (idx == -1) {
                    printf("No free slots\n");
                    close(client_fd);
                    continue;
                }

                connections[idx].client_fd = client_fd;
                add_to_poll(client_fd, POLLIN);

                printf("New connection from %s:%d\n", 
                      inet_ntoa(client_addr.sin_addr), 
                      ntohs(client_addr.sin_port));
                continue;
            }

            // Find connection for this fd
            int idx = -1;
            for (int j = 0; j < MAX_CONNECTIONS; j++) {
                if (connections[j].client_fd == fd) {
                    idx = j;
                    break;
                }
            }

            if (idx == -1) continue;

            connection_t *conn = &connections[idx];

            // Handle client data
            if (poll_fds[i].revents & POLLIN) {
                char buffer[BUFFER_SIZE];
                ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);

                if (bytes_read <= 0) {
                    close_connection(idx);
                    continue;
                }

                buffer[bytes_read] = '\0';
                printf("Received %ld bytes\n", bytes_read);

                if (!conn->is_uploading) {
                    // First chunk - extract filename and open file
                    extract_filename(buffer, conn->filename);
                    
                    conn->file_fd = open(conn->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (conn->file_fd < 0) {
                        perror("Failed to open file for upload");
                        close_connection(idx);
                        continue;
                    }

                    conn->is_uploading = true;
                    printf("Starting upload to: %s\n", conn->filename);
                }

                // Write data to file
                ssize_t bytes_written = write(conn->file_fd, buffer, bytes_read);
                if (bytes_written < 0) {
                    perror("Failed to write to file");
                    close_connection(idx);
                    continue;
                }

                conn->bytes_received += bytes_written;
                printf("Written %ld bytes to %s (total: %zu)\n", 
                      bytes_written, conn->filename, conn->bytes_received);
            }

            // Handle client ready for write (response)
            if (poll_fds[i].revents & POLLOUT) {
                const char *response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Length: 18\r\n"
                    "Connection: close\r\n\r\n"
                    "Upload successful\n";
                
                if (send(conn->client_fd, response, strlen(response), 0) < 0) {
                    perror("Failed to send response");
                }
                
                close_connection(idx);
            }
        }
    }
	// will not get here because we need to implement the signals to exit gracefully...
	close(server_fd);
    return 0;
}