#include "SocketUtils.hpp"
#include "Config.hpp"
#include "Constants.hpp"
#include "HTTPServer.hpp"
#include "ServerData.hpp"
#include "debug.h"
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
}

void setSignalHandlers() {
  signal(SIGINT, handleSignal);
  signal(SIGQUIT, handleSignal);
  signal(SIGTERM, handleSignal);
  signal(SIGHUP, handleHangup);
  signal(SIGPIPE, handlePipe);
  struct sigaction sa;
  sa.sa_handler = handleChild;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Critical flags

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Handle SIGINT signal and shotdown the server
 */
void handleSignal(int signal) {
  if (signal == SIGINT || signal == SIGQUIT || signal == SIGTERM) {
    debuglog(YELLOW, "Caught signal %d - Shutting down the server", signal);
    //   shutdownServer();
    Config::cleanup();
    std::exit(0);
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

  savedErrno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    continue;
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
// void shutdownServer() {
// 	int maxServerFd = *std::max_element(Networker::serverSockets.begin(),
// 										Networker::serverSockets.end());
// 	if (!Networker::serverSockets.empty()) {
// 	  for (std::vector<struct pollfd>::const_iterator it =
// 			   Networker::pollfds.begin();
// 		   it != Networker::pollfds.end(); ++it) {
// 		int fd = it->fd;
// 		if (fd <= maxServerFd) {
// 		  debug("Closing server socket %d\n", fd);
// 		  close(fd);
// 		} else {
// 		  if (it->revents & POLLOUT) {
// 			debug("Sending 503 Service Unavailable\n");
// 			// TODO
// 			// NetUtils::sendErrorResponse(fd, 503, "Service
// Unavailable");
// 		  }
// 		  debug("Closing client socket %d\n", fd);
// 		  close(fd);
// 		}
// 	  }
// 	}
//   }

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
  int server_socket = socket(sa.sin_family, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket == -1) {
    debug("Error - server socket: %s\n", strerror(errno));
    return -1;
  }
#endif

#ifdef __APPLE__
  int server_socket = socket(sa.sin_family, SOCK_STREAM, 0);
  if (server_socket == -1) {
    debug("Error - server socket: %s\n", strerror(errno));
    return -1;
  }
#endif

  // avoiding the address already in use error with SO_REUSEADDR
  int optval = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    debug("Error - server setsockopt: %s\n", strerror(errno));
    close(server_socket); // TODO should i close all server sockets?
    return -1;
  }
  debug("Created server socket fd: %d on port %d\n", server_socket, port);

// as per subject this code is for macos only
// Sets the server socket to non-blocking mode - retrieve the flags
#ifdef __APPLE__
  int flags = fcntl(server_socket, F_GETFL, 0);
  if (flags == -1) {
    debug("Error - fcntl F_GETFL: %s\n", strerror(errno));
    close(server_socket);
    return -1;
  }

  // so we set the flags so the socket to non-blocking
  if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
    debug("Error - fcntl F_SETFL: %s\n", strerror(errno));
    close(server_socket);
    return -1;
  }

  // Set the file descriptor flags to include FD_CLOEXEC
  int fd_flags = fcntl(server_socket, F_GETFD);
  if (fd_flags == -1) {
    debug("Error - fcntl F_GETFD: %s\n", strerror(errno));
    close(server_socket);
    return -1;
  }

  if (fcntl(server_socket, F_SETFD, fd_flags | FD_CLOEXEC) == -1) {
    debug("Error - fcntl F_SETFD: %s\n", strerror(errno));
    close(server_socket);
    return -1;
  }
#endif

  int status = bind(server_socket, (struct sockaddr *)&sa, sizeof sa);
  if (status != 0) {
    debug("Error - bind port:%d socket %d - %s\n", port, server_socket,
          strerror(errno));
    close(server_socket);
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
  int status = listen(server_socket, backlog);
  if (status != 0) {
    debug("listen error: %s\n", strerror(errno));
    close(server_socket);
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
  if (setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    debuglog(RED, "setsockopt SO_RCVTIMEO failed");
    return false;
  }

  // Set send timeout
  tv.tv_sec = Constants::responseTimeout;
  if (setsockopt(clientfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
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
  std::time_t now = std::time(NULL);
  std::vector<int> idleConnections;
  idleConnections.reserve(100);
  // lastActivityTime is a map client_fd to the last time active
  // so i get tthe serverConf for the client and check
  for (std::unordered_map<int, time_t>::iterator it =
           HTTPServer::lastActivityTime.begin();
       it != HTTPServer::lastActivityTime.end(); ++it) {

    if (now - it->second > Constants::keepalive_timeout) {
      debuglog(YELLOW,
               "[Server] Detected idle connection %d for a keep-alive timeout "
               "of % d ",
               it->first, Constants::keepalive_timeout);
      idleConnections.push_back(it->first);
    }
  }

  for (size_t i = 0; i < idleConnections.size(); ++i) {
    int fd = idleConnections[i];
    for (size_t j = 0; j < HTTPServer::pollfds.size(); ++j) {
      if (HTTPServer::pollfds[j].fd == fd) {
        debuglog(YELLOW, "Closing client socket %d", fd);
        close(fd);
        // TODO send a 408 Request Timeout response to the client before closing
        HTTPServer::remove_from_poll(fd);
        break;
      }
    }
    HTTPServer::lastActivityTime.erase(fd);
  }
  idleConnections.clear();
}

} // namespace SocketUtils

// /**
//  * @brief Get the client IP address
//  *
//  * @param clientfd The client socket file descriptor
//  * @return std::string The client IP address
//  */
// std::string NetUtils::getclient_ip(int clientfd) {
//   debugcolor(RED, "Getting client IP address for client socket %d",
//   clientfd); return Networker::remoteAddresses[clientfd];
// }

// /**
//  * @brief send a generic error response
//  *
//  * @param clientfd The client socket file descriptor
//  * @param status_code The HTTP status code
//  *
//  * We have our dedicated error response pages for each status code
//  * but this is a generic error response that will be sent if we
//  * dont have the specific error page.
//  */
// void NetUtils::sendErrorResponse(int clientfd, int status_code,
//                                  const char *status_message) {
//   std::ostringstream oss;
//   oss << "HTTP/1.1 " << status_code << " " << status_message
//       << "\r\nContent-Length: 0\r\n\r\n";
//   std::string response = oss.str();
//   debugcolor(YELLOW, "Sending error response to client socket %d %s",
//   clientfd,
//              response.c_str());
//   send(clientfd, response.c_str(), strlen(response.c_str()), 0);
// }

// /**
//  * @brief [Debug func] Convert a binary buffer to a hex string
//  *
//  * @param input The binary buffer to convert
//  * @param len The length of the buffer
//  *
//  * In input I have an array of unsigned chars, binary code like
//  *   const unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
//  * As input I do not need to pass a null terminated string, it supports
//  binary
//  * data Good for debugging. A binary buffer can be also of type uint8_t in
//  * output I will have the same but in hex like "DE AD BE EF" The function
//  return
//  * a string which will need to be freed!
//  */
// char *NetUtils::binToHex(const unsigned char *input, size_t len) {
//   char *result;

//   if (input == NULL || len <= 0) {
//     return (NULL);
//   }

//   // (2 hexits+space/chr + NULL
//   size_t resultlen = (len * 3) + 1;
//   result = new char[resultlen];
//   std::memset(result, 0, resultlen);

//   for (size_t i = 0; i < len; i++) {
//     result[i * 3] = "0123456789ABCDEF"[input[i] >> 4];
//     result[(i * 3) + 1] = "0123456789ABCDEF"[input[i] & 0x0F];
//     result[(i * 3) + 2] = ' '; // for readability
//   }
//   return (result);
// }

// std::string NetUtils::trim(const std::string &str) {
//   std::string trimmed = str;
//   std::string whitespaces = " \r\n\t";
//   size_t start = trimmed.find_first_not_of(whitespaces);
//   if (start == std::string::npos) {
//     return "";
//   }
//   size_t end = trimmed.find_last_not_of(whitespaces);
//   return trimmed.substr(start, end - start + 1);
// }
