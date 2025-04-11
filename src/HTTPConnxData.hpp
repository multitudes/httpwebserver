#pragma once

#include <iomanip>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "Config.hpp"
#include <cstring>

using std::map;
using std::string;
using std::vector;

/**
 * @brief Connection state enum
 *
 * Tracks the current state of a connection throughout its lifecycle
 */
enum ConnectionState {
  CONN_INCOMING,       // New connection, nothing processed yet
  CONN_PARSING_HEADER, // Receiving/parsing headers
  CONN_CGI,            // Processing CGI request
  CONN_FILE_REQUEST,   // Serving a file
  CONN_SIMPLE_RESPONSE,
  CONN_UPLOAD, // Handling file upload
  CONN_DONE,   // File transfer or other operation completed
  CONN_CLOSING // Ready to close
};

/**
 * @brief Tracks the state of the header parsing
 */
enum ParseStatus { PARSE_SUCCESS, PARSE_INCOMPLETE, PARSE_ERROR };

/**
 * @brief Connection state struct
 *
 * Tracks the state of a connection including request data,
 * file transfers, and CGI processing
 */
struct HTTPConnxData {
  /**
   * @brief Request data and metadata
   */
  struct ConnectionData {
    // Request parts
    string method;
    string target;
    string version;

    // Connection info
    string host;
    uint16_t port;
	char client_ip[INET_ADDRSTRLEN]; //  remoteAddress;
	

    string request;
    size_t content_length;

    // headers and cookies
    map<std::string, std::string> headers;
    map<std::string, std::string> cookies;

    bool headers_received;
    bool is_get_request;
    bool chunked;
    bool multipart;
    string boundary;
    size_t headers_end;



    // Response data
    int response_status;
    string response;
    string response_headers;
    string response_body;
    size_t bytes_sent;
    bool headers_sent;
    bool sending_response;
    bool response_sent;
    enum ParseStatus { PARSE_SUCCESS, PARSE_INCOMPLETE, PARSE_ERROR };
    ParseStatus parse_status;

    // Session management for Cookies -------------------Rufus
    string session_id;
    bool has_session;
    time_t session_created;
    time_t session_last_accessed;
    map<string, string> session_data;

    ConnectionData()
        : method(""), target(""), version(""), host(""), port(4244),
          request(""), content_length(0), headers(), cookies(),
          headers_received(false), is_get_request(false), chunked(false),
          multipart(false), boundary(""), headers_end(0), response_status(200),
          response_headers(""), response_body(""), bytes_sent(0),
          headers_sent(false), sending_response(false), response_sent(false),
          parse_status(PARSE_INCOMPLETE),
          session_id(""), has_session(false), // for session management
          session_created(0), session_last_accessed(0), session_data() // for session management
          {
			memset(client_ip, 0, sizeof(client_ip)); 
		  }
  };

  struct URLMatcherData {

    const ServerData* config;      
    string full_path;              // Full path to the requested resource
    string path_for_stat;          // Path adjusted for stat() calls
    string content_type;           // Content type (MIME type) for the response
    string file_upload_dir;             // Directory for file uploads
    bool autoindex;
    bool return_directive;         // Flag for return directive
    bool file_upload;           // Flag for file upload
	
    
    std::vector<std::string> acceptedMethods;
    URLMatcherData() : config(NULL), full_path(""), path_for_stat(""), content_type(""), file_upload_dir(""), autoindex(false), return_directive(false), file_upload(false), acceptedMethods() {}
  };

  struct CGIData {
	string buffer;
	bool is_sending;
	bool is_receiving;
	string path_info;
	string query_string;
	// CGI process ID
	pid_t child_pid;
	std::map<std::string, std::string> env;

	CGIData()
		: buffer(""), is_sending(false), is_receiving(true), child_pid(-1), path_info(""), query_string(), env() { 
			env["SERVER_SOFTWARE"] = "VibeServer/1.0";
			env["REMOTE_HOST"] = "";
			env["REMOTE_USER"] = "";
			env["GATEWAY_INTERFACE"] = "CGI/1.1";
			env["AUTH_TYPE"] = "";
			env["TRANSFER_ENCODING"] = "";
			env["PATH_INFO"] = "";
			env["PATH_TRANSLATED"] = "/";
		}
  };
  // Connection state and metadata
  ConnectionState state;

  // Request data TODO : extract out the response
  ConnectionData data;

  int client_fd;
  ssize_t indexServerConf;
  int poll_client_idx;

  // I/O state flags
  int is_sending;
  int is_receiving;
  bool headers_sent;

  // CGI processing
  int child_stdin_pipe[2];
  int child_stdout_pipe[2];
  int poll_stdin_idx;
  int poll_stdout_idx;
  pid_t child_pid;
  bool cgi_processing;

  // File handling
  int file_fd;
  long file_size;
  long file_offset;

  // Upload handling
  int writeto_fd;
  char filename[256];
  bool upload_completed;
  size_t bytes_received;

  // New fields for paths, config, and content type
  URLMatcherData urlMatcherData;
  CGIData cgiData;

  HTTPConnxData()
      : state(CONN_INCOMING), data(), client_fd(-1), indexServerConf(-1),
        is_sending(0), is_receiving(0), headers_sent(false), poll_stdin_idx(-1),
        poll_stdout_idx(-1), child_pid(-1), cgi_processing(false), file_fd(-1),
        file_size(0), file_offset(0), writeto_fd(-1), upload_completed(false),
        bytes_received(0) {
    filename[0] = '\0';
  }

  void reset();
  bool checkHeader(HTTPConnxData &state, const string &headerName,
                   string &targetVariable);
  string trim(const string &str);
  bool parsingHeaders(int client_fd, HTTPConnxData &state);
  ParseStatus parseRequestLine(const string &line);
  ParseStatus parseHeaderLine(const string &line);
  ParseStatus parseCookies(const string &cookieHeader);
  ParseStatus processContentHeaders();
  ParseStatus parseHeaders(HTTPConnxData &conn);
  ParseStatus extractPortFromHost(std::string& host, uint16_t& port);
  string formatConnectionData(const ConnectionData &data);
  string formatConnectionDataLong(const ConnectionData &data);

  // Session management methods -------------------------------------Rufus
  string generateSessionId();
  void createSession();
  bool retrieveSession();
};
