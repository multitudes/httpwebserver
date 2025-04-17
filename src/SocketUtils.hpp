#pragma once

#include "Config.hpp"
#include "ServerData.hpp"
#include "sys/socket.h"
#include <csignal>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

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
void shutdownServer();
const char *custom_inet_ntop(int af, const void *src, char *dst,
                             socklen_t size);
bool printLocalAddress(int clientfd);
bool gotPollhupShouldSkip(pollfd &currentfd);
bool gotPollerrShouldSkip(pollfd &currentfd);

} // namespace SocketUtils
