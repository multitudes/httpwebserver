#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

/**
 * @brief Connection state struct
 *
 * When sending files over multiplex I need to keep track of the state
 * of the connection and the request and how much data has been sent
 * already. If the headers have been received then I know the content lenght
 * and what to expect if the request is chunked or multipart. Also if I have a
 * GET request I dont expect a body. Amd I truncate body which is longer than
 * content length Also I can check for the maximum body size according to the
 * server configuration.
 */
struct HTTPConnection {
  struct ConnectionData {
    string method;
    string target;
    string version;
    string host;
    string request;
    size_t content_length;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> cookies;
    bool headersReceived;
    bool isGetRequest;
    bool chunked;
    bool multipart;
    string boundary;
    size_t headers_end;
    bool sending_response;
    string response;
    size_t bytes_sent;
    bool response_sent;

    ConnectionData()
        : method(""), target(""), version(""), host(""), request(""),
          content_length(0), headers(map<string, string>()),
          cookies(map<string, string>()), headersReceived(false),
          isGetRequest(false), chunked(false), multipart(false), boundary(""),
          headers_end(0), sending_response(false), response(""), bytes_sent(0),
          response_sent(false) {}
  };

  // base
  int client_fd;
  int is_sending;
  int is_receiving;
  
  // for cgi
  int child_stdin_pipe[2];  // [0] for read, [1] for write
  int child_stdout_pipe[2]; // [0] for read, [1] for write
//   int poll_stdin_idx;  // Index in poll_fds for stdin pipe
//   int poll_stdout_idx; // Index in poll_fds for stdout pipe
//   int poll_client_idx; // Index in poll_fds for client fd
  pid_t child_pid;
  
  // for simple requests
  int file_fd;
  long file_size; // off_t? is
  long file_offset;
  bool headers_sent;
  
  // for upload
  int writeto_fd;
  char filename[256]; // Store filename
  bool is_uploading;


  ConnectionData data;
  ssize_t indexServerConf;
  // if true I will send it to the cgi handler from the poll loop
  bool isIOConnx;
  // if true I will skip this pollfd in the main poll loop
  bool handledByParent;
  // when I fork a child for cgi I need to keep track of the child fd
  // to write the body to
  int childfdIN;
  // when I fork a child for cgi I need to keep track of the child fd
  // to read the response from
  int childFDOUT;
  // which pid
  pid_t cgi_pid;
  int childfdIOConnx;

  HTTPConnection()
      : client_fd(-1), is_sending(0), is_receiving(0),
		child_stdin_pipe{-1, -1}, child_stdout_pipe{-1, -1}, child_pid(-1),
		file_fd(-1), file_size(0), file_offset(0), headers_sent(false),
		writeto_fd(-1), is_uploading(false), 	  
        data(ConnectionData()), indexServerConf(-1), isIOConnx(false),
        handledByParent(false), childfdIN(-1), childFDOUT(-1), cgi_pid(0),
        childfdIOConnx(-1) {}
};
