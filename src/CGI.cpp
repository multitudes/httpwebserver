#include "CGI.hpp"
#include "HTTPServer.hpp"

namespace CGI {

// Start a CGI process for a connection
int prepareCGI(HTTPConnxData& conn) {
	std::map<std::string, std::string> env;


  // Create pipes
  debug("create pipes");
  if (pipe(conn.child_stdin_pipe) < 0) {
    perror("Failed to create pipes");
    return -1;
  }
  if (pipe(conn.child_stdout_pipe) < 0) {
	perror("Failed to create pipes");
	close(conn.child_stdin_pipe[0]);
	close(conn.child_stdin_pipe[1]);
	return -1;
  }
  debug("values in the pipes now %d", conn.child_stdin_pipe[0]);
  debug("values in the pipes now %d", conn.child_stdin_pipe[1]);
  debug("values in the pipes now %d", conn.child_stdout_pipe[0]);
  debug("values in the pipes now %d", conn.child_stdout_pipe[1]);
  // Set the pipes to non-blocking mode
  // Create child process
  pid_t pid = fork();

  if (pid < 0) {
    perror("Failed to fork");
    return -1;
  } else if (pid == 0) {
    // Child process

    // Close unused pipe ends
    close(conn.child_stdin_pipe[1]);  // Close write end of stdin pipe
    close(conn.child_stdout_pipe[0]); // Close read end of stdout pipe

    // Close original file descriptors
    close(conn.child_stdin_pipe[0]);
    close(conn.child_stdout_pipe[1]);

    // Redirect stdin and stdout
    dup2(conn.child_stdin_pipe[0], STDIN_FILENO);
    dup2(conn.child_stdout_pipe[1], STDOUT_FILENO);

	// Prepare environment variables for execve - this one a bit complicate
	// because the env expects a const char* array . the args was easier. could
	// not use the const_cast in the same way
	char** envArray = new char*[env.size() + 1];
	int i = 0;
	for (std::map<std::string, std::string>::const_iterator it = env.begin();
	it != env.end(); ++it, ++i) {
		std::string envEntry = it->first + "=" + it->second;
		envArray[i] = new char[envEntry.size() + 1];
		std::strcpy(envArray[i], envEntry.c_str());
	}
	envArray[i] = NULL;
	
	// Prepare arguments 
	std::vector<char*> args;
	std::string scriptPath = "cgi-bin/cgi_handler.py";
	args.push_back(const_cast<char*>(scriptPath.c_str()));
	args.push_back(NULL);

    // Execute the Python script
    execve("cgi-bin/cgi_handler.py", args.data(), envArray);

    // If execve fails
    perror("Failed to execute CGI script");

	// Free allocated memory if execve fails
	for (int j = 0; j < i; ++j) {
		delete[] envArray[j];
	}
	delete[] envArray;
	std::exit(EXIT_FAILURE);
  } else {
    // Parent process
    // conn.poll_stdin_idx = SocketUtils::add_to_poll(conn.child_stdin_pipe[1], POLLOUT);
    // conn.poll_stdout_idx = SocketUtils::add_to_poll(conn.child_stdout_pipe[0], POLLIN);

    // Close unused pipe ends
    close(conn.child_stdout_pipe[1]); // Close write end of stdout pipe
    close(conn.child_stdin_pipe[0]);  // Close read end of stdin pipe

			// Write the request body to the child's stdin
	conn.cgiData.buffer = conn.data.request.substr(conn.data.headers_end);
	
    conn.cgiData.child_pid = pid;
    debug("Started CGI process with PID %d", pid);
    return 0;
  }
}


} // namespace CGI