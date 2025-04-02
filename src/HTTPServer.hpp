#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <ctime>

#include "HTTPConnxData.hpp"
#include "SocketUtils.hpp"
#include "Config.hpp"
#include "ServerData.hpp"
#include "CGI.hpp"
#include "debug.h"


#define BUFFER_SIZE 4096

using std::string;
using std::vector;
using std::map;
using std::map;

namespace HTTPServer {

/**
 * testing: use the following command to test the server
 *
 * nc localhost 4244
 * curl -X POST --data-raw "This is a test" localhost:4244
 * wget -v --post-data="hello world"  http://localhost:4244
 * telnet localhost 4244
 * curl "http://localhost:4244/?sss='this%20is'"
 * curl -H "Content-Type: application/json"
 * "http://localhost:4244/?data=This%20is%20a%20test" curl -X POST -H
 * "Content-Type: application/json" --data-raw '{"message": "This is a test"}'
 * localhost:4244 curl -I -H "Content-Type: application/json"
 * http://localhost:4244
 */

typedef std::vector<struct pollfd> PollfdsVector;

extern PollfdsVector pollfds;
extern std::vector<int> serverSockets;
extern std::map<int, HTTPConnxData> connections;
extern std::map<int, std::time_t> lastActivityTime;

// Add a file descriptor to the poll array
void add_to_poll(int fd, short events);

// Remove a fd by swapping with last element (O(1))
void remove_from_poll(int fd);

// Update events for an existing fd
bool update_poll_events(int fd, short events);
// void extract_filename(const char *request, char *filename);


int run();


} // namespace HTTPServer

