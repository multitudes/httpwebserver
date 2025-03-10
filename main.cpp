#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define SERVER_PORT 4244
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 4096

// Connection structure to store client information and pipe file descriptors
typedef struct {
    int client_fd;
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

// Initialize a new connection
void init_connection(connection_t *conn) {
    conn->client_fd = -1;
    conn->child_stdin_pipe[0] = -1;
    conn->child_stdin_pipe[1] = -1;
    conn->child_stdout_pipe[0] = -1;
    conn->child_stdout_pipe[1] = -1;
    conn->child_pid = -1;
    conn->is_sending = 0;
    conn->is_receiving = 0;
}

// Add a file descriptor to the poll array
void add_to_poll(int fd, short events) {
    poll_fds[poll_fd_count].fd = fd;
    poll_fds[poll_fd_count].events = events;
    poll_fd_count++;
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
        if (connections[i].client_fd == fd) {
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
    
    // Add server socket first
    add_to_poll(server_fd, POLLIN);
    
    // Add client connections and pipes
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connection_t *conn = &connections[i];
        
        if (conn->client_fd != -1) {
            // Add client socket fd
            add_to_poll(conn->client_fd, POLLIN);
            
            // Add pipe fds if CGI process is active
            if (conn->child_pid > 0) {
                if (conn->is_sending) {
                    add_to_poll(conn->child_stdin_pipe[1], POLLOUT);
                }
                if (conn->is_receiving) {
                    add_to_poll(conn->child_stdout_pipe[0], POLLIN);
                }
            }
        }
    }
}

// Start a CGI process for a connection
int start_cgi_process(int conn_idx) {
    connection_t *conn = &connections[conn_idx];
    
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
        execve("cgi_handler.py", NULL, NULL);
        
        // If execl fails
        perror("Failed to execute CGI script");
        exit(1);
    } else {
        // Parent process
        
        // Close unused pipe ends
        close(conn->child_stdin_pipe[0]);  // Close read end of stdin pipe
        close(conn->child_stdout_pipe[1]); // Close write end of stdout pipe
        
        conn->child_pid = pid;
        conn->is_sending = 1;   // Ready to send data to CGI
        // conn->is_receiving = 1; // Ready to receive data from CGI
        
        return 0;
    }
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    
    // Initialize all connections
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        init_connection(&connections[i]);
    }
    
    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Prepare the sockaddr_in structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on any interface
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", SERVER_PORT);
    
    // Add server socket to poll
    add_to_poll(server_fd, POLLIN);
    
    // Main polling loop
    while (1) {
        update_poll_fds(server_fd);
        
        int poll_result = poll(poll_fds, poll_fd_count, -1); // Wait indefinitely
        
        if (poll_result < 0) {
            perror("Poll failed");
            break;
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
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                
                if (client_fd < 0) {
                    perror("Accept failed");
                    continue;
                }
                
                printf("New connection from %s:%d\n", 
                       inet_ntoa(client_addr.sin_addr), 
                       ntohs(client_addr.sin_port));
                
                int conn_idx = find_free_connection();
                if (conn_idx < 0) {
                    printf("No free connection slots\n");
                    close(client_fd);
                    continue;
                }
                
                // Initialize new connection
                connections[conn_idx].client_fd = client_fd;
                
                // Start CGI process for this connection
                if (start_cgi_process(conn_idx) < 0) {
                    close_connection(conn_idx);
                    continue;
                }
                
                // Update poll fds for next iteration
                continue;
            }
            
            // Handle client data
            int conn_idx = find_connection_by_fd(current_fd);
            if (conn_idx >= 0) {
                connection_t *conn = &connections[conn_idx];
                
                // Handle data from client
                if (current_fd == conn->client_fd && (poll_fds[i].revents & POLLIN)) {
                    char buffer[BUFFER_SIZE];
                    int bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE, 0);
                    
                    if (bytes_read <= 0) {
                        // Connection closed or error
                        if (bytes_read == 0) {
                            printf("Client disconnected\n");
                        } else {
                            perror("recv failed");
                        }
                        close_connection(conn_idx);
                        continue;
                    }
                    
                    printf("Received %d bytes from client\n", bytes_read);
                    
                    // Forward data to CGI process
                    if (conn->is_sending) {
                        int bytes_written = write(conn->child_stdin_pipe[1], buffer, bytes_read);
                        if (bytes_written < 0) {
                            perror("Write to CGI failed");
                            close_connection(conn_idx);
                            continue;
                        }
                    }
					if (bytes_read < BUFFER_SIZE) {
						// Close the write end of the pipe to signal EOF to the CGI process
						printf("Closing write end of pipe\n");
						close(conn->child_stdin_pipe[1]);
						conn->is_sending = 0;
						conn->is_receiving = 1;
					}
                }
				    // Check if the client has finished sending data
                
                // Handle data from CGI process (ready to write to client)
                if (conn->child_stdout_pipe[0] == current_fd && (poll_fds[i].revents & POLLIN)) {
                    char buffer[BUFFER_SIZE];
                    int bytes_read = read(conn->child_stdout_pipe[0], buffer, BUFFER_SIZE);
                    
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
                    
                    // Send CGI output back to client
                    int bytes_sent = send(conn->client_fd, buffer, bytes_read, 0);
                    if (bytes_sent < 0) {
                        perror("Send to client failed");
                        close_connection(conn_idx);
                        continue;
                    }
                    
                    printf("Sent %d bytes to client\n", bytes_sent);
                }
                
                // Handle when we can write to CGI process
                if (conn->child_stdin_pipe[1] == current_fd && (poll_fds[i].revents & POLLOUT)) {
                    // This is handled in the client data section
                    // Just mark that we're ready to send
                    printf("Ready to send data to CGI\n");
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