#include "CGI.hpp"


namespace CGI {


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
//         execve("cgi-bin/cgi_handler.py", NULL, NULL);
        
//         // If execve fails
//         perror("Failed to execute CGI script");
//         exit(1);
//     } else {
//         // Parent process
//         conn->poll_stdin_idx = add_to_poll(conn->child_stdin_pipe[1], POLLOUT);
// 		conn->poll_stdout_idx = add_to_poll(conn->child_stdout_pipe[0], POLLIN);
        
// 		// Close unused pipe ends
//         close(conn->child_stdin_pipe[0]);  // Close read end of stdin pipe
//         close(conn->child_stdout_pipe[1]); // Close write end of stdout pipe
        
//         conn->child_pid = pid;
//         conn->is_sending = 1;   // Ready to send data to CGI
//         conn->is_receiving = 0; // Ready to receive data from CGI
        
//         return 0;
//     }
// }


}	// namespace CGI