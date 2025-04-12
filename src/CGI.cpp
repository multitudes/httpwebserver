#include "CGI.hpp"
#include "HTTPServer.hpp"
#include "Utils.hpp"
#include "URLMatcher.hpp"
#include "HTTPConnxData.hpp"
#include "debug.h"

namespace CGI {

// Start a CGI process for a connection
int prepareCGI(HTTPConnxData &conn) {


  setCGIEnv(conn);
  
  // Create pipes
  debug("create pipes");
  if (pipe(conn.cgiData.child_stdin_pipe) < 0) {
    perror("Failed to create pipes");
    return -1;
  }
  if (pipe(conn.cgiData.child_stdout_pipe) < 0) {
    perror("Failed to create pipes");
    ::close(conn.cgiData.child_stdin_pipe[0]);
    close(conn.cgiData.child_stdin_pipe[1]);
    return -1;
  }
  debug("values in the pipes now %d", conn.cgiData.child_stdin_pipe[0]);
  debug("values in the pipes now %d", conn.cgiData.child_stdin_pipe[1]);
  debug("values in the pipes now %d", conn.cgiData.child_stdout_pipe[0]);
  debug("values in the pipes now %d", conn.cgiData.child_stdout_pipe[1]);
  // Set the pipes to non-blocking mode
  // Create child process
  pid_t pid = fork();

  if (pid < 0) {
    perror("Failed to fork");
    return -1;
  } else if (pid == 0) {
    // Child process

    // Close unused pipe ends
    ::close(conn.cgiData.child_stdin_pipe[1]);  // Close write end of stdin pipe
    ::close(conn.cgiData.child_stdout_pipe[0]); // Close read end of stdout pipe

    // Close original file descriptors
    ::close(conn.cgiData.child_stdin_pipe[0]);
    ::close(conn.cgiData.child_stdout_pipe[1]);

    // Redirect stdin and stdout
    ::dup2(conn.cgiData.child_stdin_pipe[0], STDIN_FILENO);
    ::dup2(conn.cgiData.child_stdout_pipe[1], STDOUT_FILENO);

    // Prepare environment variables for execve - this one a bit complicate
    // because the env expects a const char* array . the args was easier. could
    // not use the const_cast in the same way
    char **envArray = new char *[conn.cgiData.env.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = conn.cgiData.env.begin();
         it != conn.cgiData.env.end(); ++it, ++i) {
      std::string envEntry = it->first + "=" + it->second;
      envArray[i] = new char[envEntry.size() + 1];
      std::strcpy(envArray[i], envEntry.c_str());
    }
    envArray[i] = NULL;

    
    std::string script_path = removeLeadingSlash(ensureTrailinSlash(conn.urlMatcherData.config->root)) \
    + removeLeadingSlash(conn.urlMatcherData.full_path);
    debug("CGI script_path: %s", script_path.c_str());
    
    // Prepare arguments
    std::vector<char *> args;
    args.push_back(const_cast<char *>(script_path.c_str()));
    args.push_back(NULL);

    // Execute the Python script
    ::execve(script_path.c_str(), args.data(), envArray);

    // If execve fails
    ::perror("Failed to execute CGI script");

    // Free allocated memory if execve fails
    for (int j = 0; j < i; ++j) {
      delete[] envArray[j];
    }
    delete[] envArray;
    std::exit(EXIT_FAILURE);
  } else {
    // Parent process
    // conn.poll_stdin_idx = SocketUtils::add_to_poll(conn.child_stdin_pipe[1],
    // POLLOUT); conn.poll_stdout_idx =
    // SocketUtils::add_to_poll(conn.child_stdout_pipe[0], POLLIN);

    // Close unused pipe ends
    ::close(conn.cgiData.child_stdout_pipe[1]); // Close write end of stdout pipe
    ::close(conn.cgiData.child_stdin_pipe[0]);  // Close read end of stdin pipe
    
    // assign the fds to the connection data
    conn.cgiData.cgi_stdin = conn.cgiData.child_stdin_pipe[1];
    conn.cgiData.cgi_stdout = conn.cgiData.child_stdout_pipe[0];
    
    // add the fds to the poll
    conn.cgiData.is_sending = 0;
    conn.cgiData.is_receiving = 1;
    SocketUtils::add_to_poll(conn.cgiData.cgi_stdin, POLLOUT);
    SocketUtils::add_to_poll(conn.cgiData.cgi_stdout, POLLIN);

    // this buffer is bidirectional. in this case i use now for the req body
    conn.cgiData.buffer = conn.data.request.substr(conn.data.headers_end);
    debug("CGI request body: %s", conn.cgiData.buffer.c_str());
    conn.cgiData.child_pid = pid;
    debug("Started CGI process with PID %d", pid);

    // the rest will happen in the poll loop
    return 0;
  }
}

void setCGIEnv(HTTPConnxData &conn) {
  // Set environment variables for CGI
  conn.cgiData.env["SERVER_SOFTWARE"] = "VibeServer/1.0";
  conn.cgiData.env["REMOTE_HOST"] = conn.data.host;
  conn.cgiData.env["REMOTE_USER"] = "";
  conn.cgiData.env["GATEWAY_INTERFACE"] = "CGI/1.1";
  conn.cgiData.env["AUTH_TYPE"] = "";
  // for the body of the request if chunked
  if (conn.data.chunked == true) {
    conn.cgiData.env["TRANSFER_ENCODING"] = "chunked";
  } else {
    conn.cgiData.env["TRANSFER_ENCODING"] = "";
  }
  conn.cgiData.env["TRANSFER_ENCODING"] = "";
  conn.cgiData.env["REQUEST_METHOD"] = conn.data.method;
  conn.cgiData.env["SCRIPT_NAME"] = conn.urlMatcherData.full_path;
  conn.cgiData.env["PATH_INFO"] = conn.cgiData.path_info;
  conn.cgiData.env["QUERY_STRING"] = conn.cgiData.query_string;
  conn.cgiData.env["PATH_TRANSLATED"] = "/";
  conn.cgiData.env["CONTENT_TYPE"] = conn.data.headers["Content-Type"];
  conn.cgiData.env["CONTENT_LENGTH"] =
      Utils::to_string(conn.data.content_length);
  conn.cgiData.env["SERVER_NAME"] = conn.data.host;
  conn.cgiData.env["SERVER_PORT"] = Utils::to_string(conn.data.port);
}

std::string ensureTrailinSlash(std::string path) {
  if (!path.empty() && *path.rbegin() != '/') {
    path += '/';
  }
  return path;
}

std::string removeLeadingSlash(std::string path) {
  if (!path.empty() && *path.begin() == '/') {
    path.erase(0, 1);
  }
  return path;
}


} // namespace CGI