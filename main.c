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
	long file_size; // off_t? is
	long file_offset;
	bool headers_sent;
    int child_stdin_pipe[2];  // [0] for read, [1] for write
    int child_stdout_pipe[2]; // [0] for read, [1] for write
    pid_t child_pid;
    int is_sending;
    int is_receiving;
} connection_t;

// Global variables to track connections and poll fds
connection_t connections[MAX_CONNECTIONS];
struct pollfd poll_fds[MAX_CONNECTIONS * 3]; // Server socket + potentially 3 fds per client (client_fd, pipe_in, pipe_out)
int poll_fd_count = 0;

// Add a file descriptor to the poll array
void add_to_poll(int fd, short events) {
    poll_fds[poll_fd_count].fd = fd;
    poll_fds[poll_fd_count].events = events;
    poll_fd_count++;
}

/**
 * for client to server communication
 */

// Prepare to serve the index.html file
int prepare_file_response(int idx) {
    connection_t *conn = &connections[idx];
    struct stat file_stat;

    conn->file_fd = open("index.html", O_RDONLY);
    if (conn->file_fd < 0) {
        perror("Failed to open index.html");
        return -1;
    }

    if (fstat(conn->file_fd, &file_stat) < 0) {
        perror("Failed to get file stats");
        close(conn->file_fd);
        conn->file_fd = -1;
        return -1;
    }

    conn->file_size = file_stat.st_size;
    conn->file_offset = 0;
    conn->headers_sent = 0;
    add_to_poll(conn->client_fd, POLLOUT); // Poll client FD for writing
	add_to_poll(conn->file_fd, POLLIN); // Poll file FD for reading
    return 0;
}

// Send HTTP response headers
int send_response_headers(int conn_idx) {
    connection_t *conn = &connections[conn_idx];
    char headers[512];
    
    // Create HTTP response headers
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n", conn->file_size);
    
    // Send headers
    int bytes_sent = send(conn->client_fd, headers, strlen(headers), 0);
    if (bytes_sent < 0) {
        perror("Failed to send headers");
        return -1;
    }
    
    conn->headers_sent = 1;
    return bytes_sent;
}

// Send a chunk of the file to the client
int send_file_chunk(int conn_idx) {
    connection_t *conn = &connections[conn_idx];
    char buffer[BUFFER_SIZE];
    
    // Calculate bytes to read (up to buffer size)
    size_t bytes_to_read = BUFFER_SIZE;
    if (conn->file_offset + bytes_to_read > conn->file_size) {
        bytes_to_read = conn->file_size - conn->file_offset;
    }
    
    // Read from file
    ssize_t bytes_read = pread(conn->file_fd, buffer, bytes_to_read, conn->file_offset);
    if (bytes_read <= 0) {
        if (bytes_read < 0) {
            perror("Failed to read file");
        }
        return -1;
    }
    
    // Send to client
    ssize_t bytes_sent = send(conn->client_fd, buffer, bytes_read, 0);
    if (bytes_sent < 0) {
        perror("Failed to send file chunk");
        return -1;
    }
    
    // Update file offset
    conn->file_offset += bytes_sent;
    
    printf("Sent %ld bytes of file data (%ld/%ld)\n", 
           bytes_sent, conn->file_offset, conn->file_size);
    
    // Check if we've sent the entire file
    if (conn->file_offset >= conn->file_size) {
        printf("File transfer complete\n");
        return 0;  // Done sending
    }
    
    return 1;  // More data to send
}



// Initialize a new connection
void init_connection(connection_t *conn) {
    conn->client_fd = -1;
	
	// for file transfer
	conn->file_fd = -1;
    conn->file_offset = 0;
    conn->file_size = 0;
    conn->headers_sent = 0;

	//for cgi
    conn->child_stdin_pipe[0] = -1;
    conn->child_stdin_pipe[1] = -1;
    conn->child_stdout_pipe[0] = -1;
    conn->child_stdout_pipe[1] = -1;
    conn->child_pid = -1;
    conn->is_sending = 0;
    conn->is_receiving = 0;
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
			connections[i].file_fd == fd ||
            connections[i].child_stdin_pipe[1] == fd || 
            connections[i].child_stdout_pipe[0] == fd) {
            return i;
        }
    }
    return -1;
}

// Remove a connection and its associated resources
void close_connection(int conn_idx) {
    connection_t *conn = &connections[conn_idx];
    
    if (conn->client_fd != -1) {
        close(conn->client_fd);
    }
    
    if (conn->child_stdin_pipe[0] != -1) close(conn->child_stdin_pipe[0]);
    if (conn->child_stdin_pipe[1] != -1) close(conn->child_stdin_pipe[1]);
    if (conn->child_stdout_pipe[0] != -1) close(conn->child_stdout_pipe[0]);
    if (conn->child_stdout_pipe[1] != -1) close(conn->child_stdout_pipe[1]);
    
    // Kill child process if it's still running
    if (conn->child_pid > 0) {
        kill(conn->child_pid, SIGTERM);
    }
    
    // Reset the connection
    init_connection(conn);
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
        int ret = poll(poll_fds, poll_fd_count, -1);
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

            if (fd == conn->client_fd && (poll_fds[i].revents & POLLIN)) {
                char buffer[BUFFER_SIZE];
                ssize_t bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_read <= 0) {
                    printf("Client disconnected\n");
                    close_connection(idx);
                    continue;
                }
                buffer[bytes_read] = '\0';
                printf("Received %ld bytes: %s\n", bytes_read, buffer);

                if (prepare_file_response(idx) < 0) {
                    close_connection(idx);
                }
            }
            else if (fd == conn->client_fd && (poll_fds[i].revents & POLLOUT)) {
                if (!conn->headers_sent) {
                    if (send_response_headers(idx) < 0) {
                        close_connection(idx);
                    }
                }
                else if (send_file_chunk(idx) < 0) {
                    close_connection(idx);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}