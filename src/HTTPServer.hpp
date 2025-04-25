#pragma once

#include <arpa/inet.h>
#include <ctime>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Config.hpp"
#include "HTTPConnxData.hpp"
#include "ServerData.hpp"
#include "SocketUtils.hpp"
#include "debug.h"

using std::map;
using std::string;
using std::vector;
using std::set;

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

typedef vector<struct pollfd> PollfdsVector;

extern PollfdsVector pollfds;
extern vector<int> serverSockets;
extern map<int, HTTPConnxData> connections;
extern vector<ServerData> configs_;
extern set<pid_t> terminatedPids;

int run(string configFile);
void createServerSockets(const vector<ServerData> &configs,
                         vector<int> &serverSockets);
void reloadConfigFile(string configFile, vector<int> &serverSockets,
                      vector<ServerData> &configs_);
bool reload(string configFile, long currentTime);
bool checkPollErrors(pollfd fd);
bool gotServerSocketAddNewConnx(int fd);
void acceptNewClient(int server_fd);
void setSendRecTimeout(int clientfd);
bool maxConnectionsCheck(int clientfd);
void send_critical_error(int fd, int code); 
void uploadLoop(HTTPConnxData &conn, pollfd currentfd);
bool getConnectionDataByFD(int fd, HTTPConnxData*& out_conn_ptr);
void cleanupClosedConnections();

} // namespace HTTPServer
