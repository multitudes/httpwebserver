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
#include "ConfigData.hpp"
#include "CGI.hpp"
#include "debug.h"

#define SERVER_PORT 4244
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 4096

using std::string;
using std::vector;
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

extern vector<struct pollfd> pollfds;
extern vector<int> serverSockets;
extern map<int, HTTPConnxData> connections;
extern map<int, std::time_t> lastActivityTime;

extern struct pollfd
    poll_fds[MAX_CONNECTIONS * 3 + 1]; // Server socket + potentially 3 fds per
                                       // client (client_fd, pipe_in, pipe_out)
extern int poll_fd_count;

// Initialize poll_fds at startup
void init_poll_fds(int server_fd);

// Add a file descriptor to the poll array
int add_to_poll(int fd, short events);

// Find an available connection slot
int find_free_connection();

// Find the connection index for a given file descriptor
int find_connection_by_fd(int fd);

// Remove a fd by swapping with last element (O(1))
void remove_from_poll(int fd);

// Initialize a connection
void init_connection(HTTPConnxData *conn);

// Remove a connection and its associated resources
void close_connection(int conn_idx);

// Update events for an existing fd
void update_poll_events(int fd, short events);

int run();


} // namespace HTTPServer