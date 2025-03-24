#pragma once

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include "sys/socket.h"
#include <csignal>
#include "Config.hpp"
#include "ConfigData.hpp"


namespace SocketUtils {
	std::vector<ConfigData> serverConfs;
	void initialize();

	void handleSignal(int signal);
	void handleChild(int signal);
	void handleHangup(int signal);
	void handlePipe(int signal);
	void handleAlarm(int signal);
}

/**
 * @brief Network Utilities class
 *
 * Used to refactor and group network related functions
 * like creating a socket, listening on it, sending error responses
 */
class NetUtils {
 public:
//   static void initialize();
  static int createSocket(uint16_t port);
  static bool listenSocket(int server_socket);
  static const char* custom_inet_ntop(int af, const void* src, char* dst, socklen_t size);
  static std::string getclient_ip(int clientfd);
  static void setSendRecTimeout(ServerConf conf, int clientfd);
  static void checkForIdleConnections();
  static bool maxConnectionsCheck(int clientfd);
  static bool printLocalAddress(int clientfd);
  static void shutdownServer();
  static void sendErrorResponse(int clientfd, int status_code,
                                const char* status_message);
  static char* binToHex(const unsigned char* input, size_t len);
  static std::string trim(const std::string& str);

};