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
	bool response_header_sent;
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
int prepare_file_response(int conn_idx) {
    connection_t *conn = &connections[conn_idx];
    struct stat file_stat;
    
    // Open the index.html file
    conn->file_fd = open("index.html", O_RDONLY);
    if (conn->file_fd < 0) {
        perror("Failed to open index.html");
        return -1;
    }
    
    // Get file size
    if (fstat(conn->file_fd, &file_stat) < 0) {
        perror("Failed to get file stats");
        close(conn->file_fd);
        conn->file_fd = -1;
        return -1;
    }
    
    // Store file size for later use
    conn->file_size = file_stat.st_size;
    conn->file_offset = 0;
    conn->response_header_sent = 0;
    add_to_poll(conn->file_fd, POLLIN);
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
    
    conn->response_header_sent = 1;
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
    conn->response_header_sent = 0;

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
            
            // Add pipe fds if CGI process is active
            // if (conn->child_pid > 0) {
            //     // Only check for write-readiness on stdin pipe if we're sending
            //     if (conn->is_sending) {
            //         add_to_poll(conn->child_stdin_pipe[1], POLLOUT);
            //     }
                
            //     // Always check for read-readiness on stdout pipe
            //     if (conn->is_receiving) {
            //         add_to_poll(conn->child_stdout_pipe[0], POLLIN);
            //     }
            // }
        }
    }
}

// // Start a CGI process for a connection
// int start_cgi_process(int conn_idx) {
//     connection_t *conn = &connections[conn_idx];
    
//     // Create pipes
//     if (pipe(conn->child_stdin_pipe) < 0 || pipe(conn->child_stdout_pipe) < 0) {
//         perror("Failed to create pipes");
//         return -1;
//     }
    
//     // Create child process
//     pid_t pid = fork();
    
//     if (pid < 0) {
//         perror("Failed to fork");
//         return -1;
//     } else if (pid == 0) {
//         // Child process
        
//         // Close unused pipe ends
//         close(conn->child_stdin_pipe[1]);  // Close write end of stdin pipe
//         close(conn->child_stdout_pipe[0]); // Close read end of stdout pipe
        
//         // Redirect stdin and stdout
//         dup2(conn->child_stdin_pipe[0], STDIN_FILENO);
//         dup2(conn->child_stdout_pipe[1], STDOUT_FILENO);
        
//         // Close original file descriptors
//         close(conn->child_stdin_pipe[0]);
//         close(conn->child_stdout_pipe[1]);
        
//         // Execute the Python script
//         execve("cgi_handler.py", NULL, NULL);
        
//         // If execve fails
//         perror("Failed to execute CGI script");
//         exit(1);
//     } else {
//         // Parent process
        
//         // Close unused pipe ends
//         close(conn->child_stdin_pipe[0]);  // Close read end of stdin pipe
//         close(conn->child_stdout_pipe[1]); // Close write end of stdout pipe
        
//         conn->child_pid = pid;
//         conn->is_sending = 1;   // Ready to send data to CGI
//         conn->is_receiving = 0; // Ready to receive data from CGI
        
//         return 0;
//     }
// }

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
        
        int poll_result = poll(poll_fds, poll_fd_count, 10000); // Wait indefinitely
        // printf("Poll returned %d\n", poll_result);
        if (poll_result < 0) {
            perror("Poll failed");
            break;
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
                // if (start_cgi_process(conn_idx) < 0) {
                //     close_connection(conn_idx);
                //     continue;
                // }
                

                // Update poll fds for next iteration
                continue;
            }
            
            // Handle client data
            int conn_idx = find_connection_by_fd(current_fd);
            if (conn_idx >= 0) {
                connection_t *conn = &connections[conn_idx];
                
				/** this is the case of a client req a file */
				// Handle data from client
				if (current_fd == conn->client_fd && (poll_fds[i].revents & POLLIN)) {
					char buffer[BUFFER_SIZE];
					int bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE - 1, 0);
					
					if (bytes_read <= 0) {
						if (bytes_read == 0) {
							printf("Client disconnected\n");
						} else {
							perror("recv failed");
						}
						close_connection(conn_idx);
						continue;
					}
					
					// Null-terminate the request for printing
					buffer[bytes_read] = '\0';
					printf("Received request from client:\n%s\n", buffer);
					// Prepare to serve index.html
					// this is just checking the file exists
					// check the size of the file
					// open the file
					if (prepare_file_response(conn_idx) < 0) {
						close_connection(conn_idx);
						continue;
					}
				}
				if (current_fd == conn->file_fd && (poll_fds[i].revents & POLLIN)) {
						
					// First, check if we need to send headers
					if (!conn->response_header_sent) {
		
						if (send_response_headers(conn_idx) < 0) {
							close_connection(conn_idx);
							continue;
						} else {
							printf("Sent response headers\n");
							continue;
						}
					}

					// Try to send the first chunk
					if (send_file_chunk(conn_idx) < 0) {
						close_connection(conn_idx);
					}
				}


                // // Handle data from client and send to cgi
                // if (current_fd == conn->client_fd && (poll_fds[i].revents & POLLIN)) {
                    
				// 	char buffer[BUFFER_SIZE];
                //     int bytes_read = recv(conn->client_fd, buffer, BUFFER_SIZE, 0);
                    
                //     if (bytes_read <= 0) {
                //         if (bytes_read == 0) {
                //             printf("Client disconnected\n");
                //         } else {
                //             perror("recv failed");
                //         }
                //         close_connection(conn_idx);
                //         continue;
                //     }
                    
                //     printf("Received %d bytes from client\n", bytes_read);
                    
                //     // Forward data to CGI process
                    
				// 	int bytes_written = write(conn->child_stdin_pipe[1], buffer, bytes_read);
				// 	if (bytes_written < 0) {
				// 		perror("Write to CGI failed");
				// 		close_connection(conn_idx);
				// 		continue;
				// 	}
                    
					
				// 	if (bytes_read < BUFFER_SIZE) {
				// 		// Close the write end of the pipe to signal EOF to the CGI process
				// 		printf("Closing write end of pipe\n");
				// 		close(conn->child_stdin_pipe[1]);
				// 		conn->is_sending = 0;
				// 		conn->is_receiving = 1;
				// 	}
                // }
                
                // // Handle data from CGI process (ready to write to client from cgi)
                // if (conn->child_stdout_pipe[0] == current_fd && (poll_fds[i].revents & POLLIN)) {
                //     char buffer[BUFFER_SIZE];
                //     int bytes_read = read(conn->child_stdout_pipe[0], buffer, BUFFER_SIZE);
                    
                //     if (bytes_read <= 0) {
                //         // CGI process closed pipe or error
                //         if (bytes_read == 0) {
                //             printf("CGI process finished\n");
                //             conn->is_receiving = 0;
                //         } else {
                //             perror("Read from CGI failed");
                //             close_connection(conn_idx);
                //         }
                //         continue;
                //     }
				// 	printf("Received %d bytes from cgi\n", bytes_read);
					
				// 		// Send CGI output back to client
				// 	int bytes_sent = send(conn->client_fd, buffer, bytes_read, 0);
				// 	if (bytes_sent < 0) {
				// 		perror("Send to client failed");
				// 		close_connection(conn_idx);
				// 		continue;
				// 	}
    
    			// 	printf("Sent %d bytes to client\n", bytes_sent);

									
				// 	// Close the connection after sending the response
				// 	if (bytes_read < BUFFER_SIZE) {
				// 		printf("Closing client connection\n");
				// 		close_connection(conn_idx);
				// 	}
				// }


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