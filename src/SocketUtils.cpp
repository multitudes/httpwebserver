#include "SocketUtils.hpp"
#include "Config.hpp"
#include "Constants.hpp"
#include "HTTPServer.hpp"
#include "ServerData.hpp"
#include "debug.h"
#include <algorithm>
#include <csignal>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using std::signal;

namespace SocketUtils {

// Add a file descriptor to the poll array
void add_to_poll(int fd, short events) {
  struct pollfd pfd;
  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;
  pfd.events = events;
  HTTPServer::pollfds.push_back(pfd);
}

// Remove a fd by swapping with last element (O(1))
void remove_from_poll(int fd) {
  for (HTTPServer::PollfdsVector::iterator it = HTTPServer::pollfds.begin();
       it != HTTPServer::pollfds.end(); ++it) {
    if (it->fd == fd) {
      *it = HTTPServer::pollfds.back();
      HTTPServer::pollfds.pop_back();
      return;
    }
  }
}

/**
 * @brief Initialize the webserver
 *
 * This function initializes creating and binding the server
 * sockets, setting up the pollfd array, and setting up the signal handler for
 * SIGINT.
 * It will throw a runtime error if I could not create the server socket. If the
 * socket could not be bound to the port or could not be set to listening mode.
 */
void initialize() {
  setSignalHandlers();
  Constants::initStatusMessageMap();
  Constants::initMimeTypes();
  // for performance reasons, I reserve space in the vectors first
  // so that they do not have to be resized
  HTTPServer::serverSockets.reserve(10);
  HTTPServer::pollfds.reserve(100);
}

void setSignalHandlers() {
  /* SIGINT is ctrl+c
  SIGQUIT is ctrl+\
  SIGTERM is kill
  SIGHUP is terminal hangup
  SIGPIPE is write to a socket that has been closed
  SIGCHLD is child process terminated
  */
  signal(SIGINT, handleSignal);
  signal(SIGQUIT, handleSignal);
  signal(SIGTERM, handleSignal);
  signal(SIGHUP, handleHangup);
  signal(SIGPIPE, handlePipe);

  struct sigaction sa;
  sa.sa_handler = handleChild;
  sigemptyset(&sa.sa_mask);
  /*
  - SA_RESTART flag, interrupted system calls (e.g., read, write, accept)
        will automatically restart instead of failing with EINTR.
  - SA_NOCLDSTOP: Prevents the signal from being triggered when
        child processes stop or continue (only triggers on termination). */
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    shutdownServer();
    debuglog(RED, "Error setting up SIGCHLD handler - exiting");
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Handle SIGINT signal and shotdown the server
 */
void handleSignal(int signal) {
  if (signal == SIGINT || signal == SIGQUIT || signal == SIGTERM) {
    debuglog(YELLOW, "Caught signal %d - Shutting down the server", signal);
    shutdownServer();
    Config::cleanup();
    std::exit(EXIT_SUCCESS);
  }
}

/**
 * @brief Handle child process termination
 *
 * This function is called when a child process terminates
 * waitpid might change errno so I save it and restore it
 * the WNOHANG option is used to return immediately if no
 * child has exited
 */
void handleChild(int signal) {
  debuglog(YELLOW, "Received SIGCHLD signal %d, reaping child ...", signal);
  int savedErrno;
  pid_t pid;

  savedErrno = errno;
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    HTTPServer::terminatedPids.insert(pid);
    debuglog(YELLOW, "Child process %d terminated", pid);
    debug("Child process %d terminated", pid);
    continue;
  }
  errno = savedErrno;
}

/**
 * @brief Handle SIGHUP signal
 *
 * Sent when the controlling terminal is closed.
 * Common Usage: Often used to signal a process to reload its configuration.
 */
void handleHangup(int signal) {
  debug("Received SIGHUP signal %d, reloading configuration...", signal);
  // TODO insert what to do here
}

/**
 * @brief Handle SIGPIPE signal
 *
 * It will be ignored. I will get a EPIPE error when writing to a socket
 * that has been closed by the client
 */
void handlePipe(int signal) {
  debuglog(RED, "Received SIGPIPE signal %d, ignoring...", signal);
}

/**
 * @brief Handle SIGALRM signal
 *
 * This signal is sent when a timeout with the alarm(timeout) function
 * has occurred.
 */
void handleAlarm(int signal) {
  debuglog(RED, "Received SIGALRM signal %d for cgi timeout", signal);
}

/**
 * @brief Shutdown the server
 *
 * This function shuts down the server by closing all server sockets and client
 * sockets.
 */
void shutdownServer() {
  // Close all server sockets first
  for (std::vector<int>::const_iterator it = HTTPServer::serverSockets.begin();
       it != HTTPServer::serverSockets.end(); ++it) {
    debuglog(YELLOW, "Closing server socket %d\n", *it);
    shutdown(*it, SHUT_RDWR);
    close(*it);
    remove_from_poll(*it);
  }
  // Close all client connections but make a copy
  HTTPServer::PollfdsVector pollfds_copy = HTTPServer::pollfds;

  for (HTTPServer::PollfdsVector::const_iterator it = pollfds_copy.begin();
       it != pollfds_copy.end(); ++it) {
    int fd = it->fd;

    debuglog(YELLOW, "Closing client socket %d\n", fd);
    debug("Closing client socket %d\n", fd);

    close(fd);

    // Clean up associated resources
    if (HTTPServer::connections.find(fd) != HTTPServer::connections.end()) {
      HTTPConnxData &conn = HTTPServer::connections[fd];
      conn.reset(); // reset the connection data
    }
  }

  // Clear all data structures
  HTTPServer::pollfds.clear();
  pollfds_copy.clear();
  HTTPServer::serverSockets.clear();
  HTTPServer::connections.clear();
  HTTPServer::terminatedPids.clear();
  debuglog(YELLOW, "Server shutdown complete.");
}

/**
 * @brief Create a socket and bind it to a port
 *
 * @param port The port to bind the socket to
 * @return int The file descriptor of the created socket
 */
int createBindSocket(uint16_t port) {
  struct sockaddr_in sa;
  std::memset(&sa, 0, sizeof sa);
  sa.sin_family = AF_INET;
  // Bind to all available interfaces - INADDR_ANY
  sa.sin_addr.s_addr = htonl(INADDR_ANY);
  sa.sin_port = htons(port);

#ifdef __linux__
  int server_socket = ::socket(sa.sin_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket == -1) {
    debug("Error - server socket: %s\n", strerror(errno));
    return -1;
  }
#endif

#ifdef __APPLE__
  int server_socket = ::socket(sa.sin_family, SOCK_STREAM, 0);
  if (server_socket == -1) {
    debug("Error - server socket: %s\n", strerror(errno));
    return -1;
  }
#endif

  // avoiding the address already in use error with SO_REUSEADDR
  int optval = 1;
  if (::setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval)) == -1) {
    debug("Error - server setsockopt: %s\n", strerror(errno));
    ::close(server_socket); // TODO should i close all server sockets?
    return -1;
  }
  debug("Created server socket fd: %d on port %d\n", server_socket, port);

// as per subject this code is for macos only
// Sets the server socket to non-blocking mode - retrieve the flags
#ifdef __APPLE__
  int flags = ::fcntl(server_socket, F_GETFL, 0);
  if (flags == -1) {
    debug("Error - fcntl F_GETFL: %s\n", strerror(errno));
    ::close(server_socket);
    return -1;
  }

  // so we set the flags so the socket to non-blocking
  if (::fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
    debug("Error - fcntl F_SETFL: %s\n", strerror(errno));
    ::close(server_socket);
    return -1;
  }

  // Set the file descriptor flags to include FD_CLOEXEC
  int fd_flags = fcntl(server_socket, F_GETFD);
  if (fd_flags == -1) {
    debug("Error - fcntl F_GETFD: %s\n", strerror(errno));
    ::close(server_socket);
    return -1;
  }

  if (::fcntl(server_socket, F_SETFD, fd_flags | FD_CLOEXEC) == -1) {
    debug("Error - fcntl F_SETFD: %s\n", strerror(errno));
    ::close(server_socket);
    return -1;
  }
#endif

  int status = ::bind(server_socket, (struct sockaddr *)&sa, sizeof sa);
  if (status != 0) {
    debug("Error - bind port:%d socket %d - %s\n", port, server_socket,
          strerror(errno));
    ::close(server_socket);
    return -1;
  }
  debug("Bound server_socket [%d] to localhost port %d\n", server_socket, port);
  return server_socket;
}

/**
 * @brief Listen on a socket
 *
 * @param server_socket The server socket file descriptor
 * @return bool True if the socket is listening, false if an error occurred
 */
bool listenSocket(int server_socket) {
  int backlog = 10;
  int status = ::listen(server_socket, backlog);
  if (status != 0) {
    debug("listen error: %s\n", strerror(errno));
    ::close(server_socket);
    return false;
  }
  debug("Listening on localhost server fd %d\n", server_socket);
  return true;
}

/**
 * @brief Set the send and receive timeouts for a client socket
 *
 * @param conf The server configuration
 * @param clientfd The client socket file descriptor
 *
 * I use the values in the server configuration to set the send and receive
 * timeouts for the client socket. This is useful for handling slow or
 * unresponsive clients.
 */
bool setSendRecTimeout(int clientfd) {
  // connections timeout rcvd
  struct timeval tv;
  tv.tv_sec = Constants::requestTimeout;
  tv.tv_usec = 0;
  if (::setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    debuglog(RED, "setsockopt SO_RCVTIMEO failed");
    return false;
  }

  // Set send timeout
  tv.tv_sec = Constants::responseTimeout;
  if (::setsockopt(clientfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
    debuglog(RED, "setsockopt SO_SNDTIMEO failed"); // will not work in cgi
    return false;
  }
  debuglog(YELLOW, "Set send/receive timeout for client socket %d", clientfd);
  return true;
}

/**
 * @brief Check for idle connections
 *
 * This function checks for idle connections by iterating through the
 * lastActivityTime map
 * TODO send a 408 Request Timeout response to the client before closing
 */
void checkForIdleConnections() {
  std::time_t now = std::time(NULL); // seconds since epoch!
  std::map<int, HTTPConnxData>::iterator it = HTTPServer::connections.begin();
  // only clientfds have to be in the lastActivityTime map
  // so we can safely? erase them
  while (it != HTTPServer::connections.end()) {
    HTTPConnxData &conn = it->second;

    // Check idle time
    if (now - conn.data.lastActivityTime > Constants::keepalive_timeout) {
      debuglog(YELLOW, "Closing idle connection (fd %d)", conn.client_fd);
      debug("Closing idle connection (fd %d)", conn.client_fd);
      conn.reset();
      SocketUtils::remove_from_poll(conn.client_fd);
      close(conn.client_fd);
      conn.client_fd = -1; // Mark as closed
    }
    ++it; // Move to next connection
  }
}

/**
 * @brief Custom inet_ntop implementation for IPv4 addresses
 *
 * @param af The address family
 * @param src The source address
 * @param dst The destination buffer
 * @param size The size of the buffer
 * @return const char* The string representation of the address
 *
 * This function is a custom implementation of inet_ntop for IPv4 addresses.
 * It takes an address family, a source address, a destination buffer, and the
 * size of the buffer. It returns the string representation of the address.
 * The reason for this custom implementation is that inet_ntop is allowed in the
 * subject for webserv and it is a simple function to implement. Also
 * considering that we develop for linux and macos only.
 */
const char *custom_inet_ntop(int af, const void *src, char *dst,
                             socklen_t size) {
  if (af == AF_INET) {
    const struct in_addr *addr = static_cast<const struct in_addr *>(src);
    unsigned char *bytes = (unsigned char *)&addr->s_addr;
    ::snprintf(dst, size, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2],
               bytes[3]);
    return dst;
  }
  errno = EAFNOSUPPORT; // Address family not supported error
  return NULL;
}

/**
 * @brief Print the local address of the client
 *
 * @param clientfd The client socket file descriptor
 * @return bool True if the address was printed successfully, false otherwise
 *
 * The local address refers to the IP address and port number assigned to the
 * server's socket on the local machine. This is the address that the server
 * uses to listen for incoming connections from clients.
 */
bool printLocalAddress(int clientfd) {
  struct sockaddr_in local_addr;
  socklen_t addr_len = sizeof(local_addr);
  if (::getsockname(clientfd, (struct sockaddr *)&local_addr, &addr_len) ==
      -1) {
    debug("[Server] getsockname error: %s\n", strerror(errno));
    return false;
  }
  uint16_t local_port = ntohs(local_addr.sin_port);
  debug("[Server] Accepted new connection on client socket %d, port %d",
        clientfd, local_port);
  return true;
}

bool gotPollhupShouldSkip(pollfd &currentfd) {
  // Exception: POLLERR, POLLHUP, and POLLNVAL can be returned even if not
  // requested
  if (currentfd.revents & POLLHUP) {
    debuglog(RED, "POLLHUP Connection on fd %d ", currentfd.fd);
    debug("POLLHUP Connection on fd %d ", currentfd.fd);
    // Now safely get reference to the connection data
    std::map<int, HTTPConnxData>::iterator conn_it =
        HTTPServer::connections.find(currentfd.fd);

    if (conn_it == HTTPServer::connections.end()) {
      for (std::map<int, HTTPConnxData>::iterator it =
               HTTPServer::connections.begin();
           it != HTTPServer::connections.end(); ++it) {
        if (it->second.cgiData.cgi_stdin_fd == currentfd.fd ||
            it->second.cgiData.cgi_stdout_fd == currentfd.fd) {
          conn_it = it;
          break;
        }
      }
      // Still not found in connections? remove it
      debug("FD %d not found in connections - removing", currentfd.fd);
      SocketUtils::remove_from_poll(currentfd.fd);
      close(currentfd.fd);
      return true;
    }

    HTTPConnxData &conn = conn_it->second;

    if (currentfd.fd == conn.client_fd) {
      debug("Closing and erasing the connection %d from the map", currentfd.fd);
      debug("POLLHUP on client fd %d - total number of connx %ld - size of "
            "pollfds %ld ",
            currentfd.fd, HTTPServer::connections.size(),
            HTTPServer::pollfds.size());
      conn.reset();
      SocketUtils::remove_from_poll(conn.client_fd);
      close(conn.client_fd);
      conn.client_fd = -1; // Mark as closed
      return true;
    }

    // non fatal pollhups
    if (currentfd.fd == conn.cgiData.cgi_stdout_fd) {
      debug("POLLHUP on CGI stdout pipe %d", conn.cgiData.cgi_stdout_fd);
      close(conn.cgiData.cgi_stdout_fd);
      SocketUtils::remove_from_poll(conn.cgiData.cgi_stdout_fd);
      conn.cgiData.cgi_stdout_fd = -1; // Mark as closed
    }
  }
  return false;
}

bool gotPollerrShouldSkip(pollfd &currentfd) {
  if (currentfd.revents & (POLLERR | POLLNVAL)) {
    debuglog(RED, "Error condition on fd %d", currentfd.fd);
    int error = 0;
    socklen_t len = sizeof(error);

    // Now safely get reference to the connection data
    std::map<int, HTTPConnxData>::iterator conn_it =
        HTTPServer::connections.find(currentfd.fd);

    if (conn_it == HTTPServer::connections.end()) {
      for (std::map<int, HTTPConnxData>::iterator it =
               HTTPServer::connections.begin();
           it != HTTPServer::connections.end(); ++it) {
        if (it->second.cgiData.cgi_stdin_fd == currentfd.fd ||
            it->second.cgiData.cgi_stdout_fd == currentfd.fd) {
          conn_it = it;
          break;
        }
      }
      // Still not found? remove it
      debug("FD %d not found in connections - removing", currentfd.fd);
      SocketUtils::remove_from_poll(currentfd.fd);
      close(currentfd.fd);
      return true;
    }

    HTTPConnxData &conn = conn_it->second;

    if (currentfd.fd == conn.client_fd) {
      // unusable connection
      debug("Closing client fd %d", currentfd.fd);
      conn.reset();
      SocketUtils::remove_from_poll(conn.client_fd);
      close(conn.client_fd);
      conn.client_fd = -1; // Mark as closed
    }
    if (currentfd.fd == conn.cgiData.cgi_stdin_fd) {
      debug("Closing CGI stdin pipe %d", currentfd.fd);
      close(conn.cgiData.cgi_stdin_fd);
      SocketUtils::remove_from_poll(conn.cgiData.cgi_stdin_fd);
      conn.cgiData.cgi_stdin_fd = -1; // Mark as closed
    } else if (currentfd.fd == conn.cgiData.cgi_stdout_fd) {
      debug("Closing CGI stdout pipe %d", currentfd.fd);
      close(conn.cgiData.cgi_stdout_fd);
      SocketUtils::remove_from_poll(conn.cgiData.cgi_stdout_fd);
      conn.cgiData.cgi_stdout_fd = -1; // Mark as closed
    }

    return true;
  }
  return false;
}

} // namespace SocketUtils
