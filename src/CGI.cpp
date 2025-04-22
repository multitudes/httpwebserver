#include "CGI.hpp"
#include "Config.hpp"
#include "HTTPConnxData.hpp"
#include "HTTPServer.hpp"
#include "URLMatcher.hpp"
#include "Utils.hpp"
#include "debug.h"

namespace CGI {

// Start a CGI process for a connection
int prepareCGI(HTTPConnxData &conn) {
  // CLEAN UP PREVIOUS PIPES IF THEY EXIST
  if (conn.cgiData.child_stdin_pipe[0] != -1) {
      ::close(conn.cgiData.child_stdin_pipe[0]);
      SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[0]);
      conn.cgiData.child_stdin_pipe[0] = -1;
  }
  if (conn.cgiData.child_stdin_pipe[1] != -1) {
      ::close(conn.cgiData.child_stdin_pipe[1]);
      SocketUtils::remove_from_poll(conn.cgiData.child_stdin_pipe[1]);
      conn.cgiData.child_stdin_pipe[1] = -1;
  }
  if (conn.cgiData.child_stdout_pipe[0] != -1) {
      ::close(conn.cgiData.child_stdout_pipe[0]);
      SocketUtils::remove_from_poll(conn.cgiData.child_stdout_pipe[0]);
      conn.cgiData.child_stdout_pipe[0] = -1;
  }
  if (conn.cgiData.child_stdout_pipe[1] != -1) {
      ::close(conn.cgiData.child_stdout_pipe[1]);
      SocketUtils::remove_from_poll(conn.cgiData.child_stdout_pipe[1]);
      conn.cgiData.child_stdout_pipe[1] = -1;
  }

  setCGIEnv(conn);

  // Just get the request body - chunking already handled in URLMatcher
  conn.cgiData.buffer = conn.data.request.substr(conn.data.headers_end);

  // Create pipes
  debug("create pipes");
  if (pipe(conn.cgiData.child_stdin_pipe) < 0) {
    perror("Failed to create pipes");
    return -1;
  }
  if (pipe(conn.cgiData.child_stdout_pipe) < 0) {
    perror("Failed to create pipes");
    ::close(conn.cgiData.child_stdin_pipe[0]);
    ::close(conn.cgiData.child_stdin_pipe[1]);
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
    ::close(conn.cgiData.child_stdin_pipe[1]); // Close write end of stdin pipe
    ::close(conn.cgiData.child_stdout_pipe[0]); // Close read end of stdout pipe

    // Redirect stdin and stdout
    ::dup2(conn.cgiData.child_stdin_pipe[0], STDIN_FILENO);
    ::dup2(conn.cgiData.child_stdout_pipe[1], STDOUT_FILENO);

    // Close original file descriptors
    ::close(conn.cgiData.child_stdin_pipe[0]);
    ::close(conn.cgiData.child_stdout_pipe[1]);

    // Prepare environment variables for execve - this one a bit complicate
    // because the env expects a const char* array . the args was easier. could
    // not use the const_cast in the same way
    char **envArray = new char *[conn.cgiData.env.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it =
             conn.cgiData.env.begin();
         it != conn.cgiData.env.end(); ++it, ++i) {
      std::string envEntry = it->first + "=" + it->second;
      envArray[i] = new char[envEntry.size() + 1];
      std::strcpy(envArray[i], envEntry.c_str());
    }
    envArray[i] = NULL;

    std::string script_path = removeLeadingSlash(ensureTrailinSlash(
                                  conn.urlMatcherData.config->root)) +
                              removeLeadingSlash(conn.urlMatcherData.full_path);
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
    // Close unused pipe ends
    ::close(conn.cgiData.child_stdout_pipe[1]);    // Close write end of stdout pipe
    ::close(conn.cgiData.child_stdin_pipe[0]); // Close read end of stdin pipe


    // for clarity I will assign the fds to the connection data cgi
    conn.cgiData.cgi_stdin_fd = conn.cgiData.child_stdin_pipe[1];
    conn.cgiData.cgi_stdout_fd = conn.cgiData.child_stdout_pipe[0];
    // assign the fds to the connection data
    if (conn.data.method == "GET" || (conn.data.content_length == 0)) {
      // No data to send to CGI stdin, close the write end of the pipe
      debug("GET request in cgi - closing child stdin pipe[1]");
      conn.state = CONN_CGI_SENDING;
      SocketUtils::add_to_poll(conn.cgiData.child_stdout_pipe[0], POLLIN);
      ::close(conn.cgiData.child_stdin_pipe[1]);
    } else {
      SocketUtils::add_to_poll(conn.cgiData.child_stdin_pipe[1], POLLOUT);
      SocketUtils::add_to_poll(conn.cgiData.child_stdout_pipe[0], POLLIN);
    }

    // add the fds to the poll

    // this buffer is bidirectional. in this case i use now for the req body
    debug("CGI request body: %s", conn.cgiData.buffer.c_str());
    conn.cgiData.child_pid = pid;
    debug("Started CGI process with PID %d", pid);

    // the rest will happen in the poll loop
    return 0;
  }
}

void setCGIEnv(HTTPConnxData &conn) {
  // get the config for the connection
  const ServerData *conf = Config::getConfigByPort(conn.data.port);
  if (conf == NULL) {
    debug("No config found for port %d", conn.data.port);
    throw std::runtime_error(
        "No config found for port " + Utils::to_string(conn.data.port));
    return;
  }

  conn.cgiData.env.clear();
  // Set environment variables for CGI - some are already init to defaults
  // int he struct constructor - ex REMOTE_USER which we dont use
  conn.cgiData.env["UPLOAD_DIR"] = conf->cgiData.upload_dir;
  debug("set upload dir for cgi to %s", conn.cgiData.env["UPLOAD_DIR"].c_str());
  conn.cgiData.env["REMOTE_HOST"] = conn.data.host;
  debug("set remote host to %s", conn.cgiData.env["REMOTE_HOST"].c_str());
  // for the body of the request if chunked
  debug("it is chunked %d", conn.data.chunked);
  conn.cgiData.env["HTTP_TRANSFER_ENCODING"] =
      conn.data.headers["Transfer-Encoding"];
  debug("set transfer encoding to %s",
        conn.cgiData.env["HTTP_TRANSFER_ENCODING"].c_str());
  conn.cgiData.env["REQUEST_METHOD"] = conn.data.method;
  debug("set request method to %s", conn.cgiData.env["REQUEST_METHOD"].c_str());
  conn.cgiData.env["SCRIPT_NAME"] = conn.cgiData.script_name;
  debug("set script name to %s", conn.cgiData.env["SCRIPT_NAME"].c_str());
  conn.cgiData.env["PATH_INFO"] =
      conn.cgiData.path_info.empty() ? "/" : conn.cgiData.path_info;
  debug("set path info to %s", conn.cgiData.env["PATH_INFO"].c_str());
  conn.cgiData.env["QUERY_STRING"] = conn.cgiData.query_string;
  debug("set query string to %s", conn.cgiData.env["QUERY_STRING"].c_str());

  string path_translated =
      ensureTrailinSlash(conn.urlMatcherData.config->root) +
      removeLeadingSlash(conn.cgiData.path_info);
  conn.cgiData.env["PATH_TRANSLATED"] = path_translated;
  debug("set path translated to %s",
        conn.cgiData.env["PATH_TRANSLATED"].c_str());

  // Ensure Content-Type is always set
  debug("content type in urlmatcher %s",
        conn.urlMatcherData.content_type.c_str());
  debug("content type in general %s",
        conn.data.headers["Content-Type"].c_str());
  conn.cgiData.env["CONTENT_TYPE"] = conn.data.headers["Content-Type"];
  if (conn.data.content_length > 0) {
    conn.cgiData.env["CONTENT_LENGTH"] =
        Utils::to_string(conn.data.content_length);
  }
  debug("set content length to %s", conn.cgiData.env["CONTENT_LENGTH"].c_str());
  debug("content length in data? %zu", conn.data.content_length);
  conn.cgiData.env["SERVER_NAME"] = conn.data.host;
  debug("set server name to %s", conn.cgiData.env["SERVER_NAME"].c_str());
  conn.cgiData.env["SERVER_PORT"] = Utils::to_string(conn.data.port);
  debug("set server port to %s", conn.cgiData.env["SERVER_PORT"].c_str());
  conn.cgiData.env["SERVER_PROTOCOL"] = "HTTP/1.1";
  debug("set server protocol to %s",
        conn.cgiData.env["SERVER_PROTOCOL"].c_str());
  conn.cgiData.env["REMOTE_ADDR"] = conn.client_ip;
  debug("set remote addr to %s", conn.cgiData.env["REMOTE_ADDR"].c_str());
  conn.cgiData.env["SERVER_SOFTWARE"] = "VibeServer/1.0";
  debug("set server software to %s",
        conn.cgiData.env["SERVER_SOFTWARE"].c_str());
  conn.cgiData.env["GATEWAY_INTERFACE"] = "CGI/1.1";
  conn.cgiData.env["REMOTE_USER"] = "N/A";
  debug("set remote user to %s", conn.cgiData.env["REMOTE_USER"].c_str());
  conn.cgiData.env["AUTH_TYPE"] = "N/A";
  debug("set auth type to %s", conn.cgiData.env["AUTH_TYPE"].c_str());
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

/**
 *
 * testing
 *
 *
 curl -v -X POST -H "Content-Type: application/x-www-form-urlencoded" \
-d "delete_files=egyptiancatsuploadtest.jpeg" \
http://localhost:4244/cgi/delete.py
 */