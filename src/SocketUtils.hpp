#pragma once

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include "sys/socket.h"
#include <csignal>
#include "Config.hpp"
#include "ServerData.hpp"


namespace SocketUtils {
	void initialize();
	void setSignalHandlers();
	void handleSignal(int signal);
	void handleChild(int signal);
	void handleHangup(int signal);
	void handlePipe(int signal);
	void handleAlarm(int signal);

	int createBindSocket(uint16_t port);
	bool listenSocket(int server_socket);
	bool setSendRecTimeout(int clientfd);
	void checkForIdleConnections();
	void add_to_poll(int fd, short events);
	void remove_from_poll(int fd);
	bool update_poll_events(int fd, short events);
	void shutdownServer();
}
