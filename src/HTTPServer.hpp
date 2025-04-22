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

#define BUFFER_SIZE 8192

using std::map;
using std::string;
using std::vector;

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
extern vector<ServerData> configs_;
extern std::set<pid_t> terminatedPids;

int run(std::string configFile);
void createServerSockets(const vector<ServerData> &configs,
                         vector<int> &serverSockets);
void reloadConfigFile(std::string configFile, vector<int> &serverSockets,
                      vector<ServerData> &configs_);
bool reload(string configFile, long currentTime);
bool checkPollErrors(pollfd fd);
bool gotServerSocketAddNewConnx(int fd);
void acceptNewClient(int server_fd);
void setSendRecTimeout(int clientfd);
bool maxConnectionsCheck(int clientfd);
void send_critical_error(int fd, int code); 
bool uploadComplete(HTTPConnxData &conn); 
bool writingFirstPayloadCompletesUpload(HTTPConnxData &conn);
bool readFromClientForUpload(HTTPConnxData &conn);
bool writeUploadToFile(HTTPConnxData &conn);
bool finishedSendingSimpleResponse(HTTPConnxData &conn);
bool settingHeadersIfNeeded(HTTPConnxData &conn); 
bool readNewDataFromFile(HTTPConnxData &conn);
bool sendNewDataFromFileToClient(HTTPConnxData &conn);
void checkCompletionConditions(HTTPConnxData &conn);
void uploadLoop(HTTPConnxData &conn, pollfd currentfd);
void write_to_child_stdin(HTTPConnxData &conn, int current_fd, int pollfd);
void read_from_client_into_buffer(HTTPConnxData &conn, int current_fd); 
void write_to_client_from_cgi(HTTPConnxData &conn, int current_fd);
void check_for_client_timeout(HTTPConnxData &conn);
} // namespace HTTPServer
