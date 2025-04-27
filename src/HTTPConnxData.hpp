#pragma once

#include "Config.hpp"
#include <cstring>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

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
  CONN_PARSING_HEADER, 
  CONN_CGI_INCOMING,           
  CONN_CGI_FINISHED,
  CONN_CGI_SENDING,
  CONN_FILE_REQUEST,   // Serving a file
  CONN_SIMPLE_RESPONSE,
  CONN_UPLOAD, 
  CONN_RECV_CHUNKS // Receiving chunked data
};

/**
 * @brief Tracks the state of the header parsing
 */
enum ParseStatus { HEADERS_PARSE_SUCCESS, HEADERS_PARSE_INCOMPLETE, HEADERS_PARSE_ERROR };

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
    
    string request;
    size_t content_length;
    
    // headers and cookies
    map<std::string, std::string> headers;
    map<std::string, std::string> cookies;
    
    bool headers_received;
    vector<char> buffer;
    bool chunked;
    string chunkedBody;
    
    bool multipart;
    string boundary;
    size_t headers_end;
    
    // Response data
    int response_status;
    string response;
    string response_headers;
    size_t bytes_sent;
    bool sending_response;
    bool response_sent;

    // enum to keep trackof the header parsing state
    enum ParseStatus { HEADERS_PARSE_SUCCESS, HEADERS_PARSE_INCOMPLETE, HEADERS_PARSE_ERROR };
    ParseStatus parse_status;

    // Session management for Cookies -------------------Rufus
    string session_id;
    bool has_session;
    time_t session_created;
    time_t session_last_accessed;
    map<string, string> session_data;

    time_t lastActivityTime;
    time_t client_timeout;

    ConnectionData()
        : method(""), target(""), version(""), host(""), port(4244),
          request(""), content_length(0), headers(), cookies(),
          headers_received(false), chunked(false), chunkedBody(""),
          multipart(false), boundary(""), headers_end(0), response_status(200),
          response_headers(""), bytes_sent(0),
          sending_response(false), response_sent(false),
          parse_status(HEADERS_PARSE_INCOMPLETE), session_id(""),
          has_session(false), // for session management
          session_created(0), session_last_accessed(0),
          session_data(), // for session management
          lastActivityTime(std::time(NULL)),
          client_timeout(0)  
    {}
  };

  struct URLMatcherData {

    string target;
    string full_path;     // Full path to the requested resource
    string path_for_stat; // Path adjusted for stat() calls
    string content_type;  // Content type (MIME type) for the response
    long file_size; // not size_t because of stat() return type
    bool autoindex;
    bool return_directive; // Flag for return directive
    bool file_upload;
    bool cookie; // Flag for file upload

    std::vector<std::string> acceptedMethods;
    URLMatcherData()
        : full_path(""), path_for_stat(""), content_type(""), file_size(0),
          autoindex(false), return_directive(false), file_upload(false),
          cookie(false), acceptedMethods() {}
  };

  struct CGIData {
    string buffer;
    string script_name;
    string path_info;
    string query_string;
    pid_t child_pid;
    std::map<std::string, std::string> env;

    // CGI processing
    int child_stdin_pipe[2];
    int child_stdout_pipe[2];
    int cgi_stdin_fd;
    int cgi_stdout_fd;
    size_t bytes_received;
    std::time_t child_timeout;

    CGIData()
        : buffer(""), script_name(""), path_info(""), query_string(),
          child_pid(-1), env(), cgi_stdin_fd(-1), cgi_stdout_fd(-1), 
          bytes_received(0), child_timeout(0) {

      child_stdin_pipe[0] = -1;
      child_stdin_pipe[1] = -1;
      child_stdout_pipe[0] = -1;
      child_stdout_pipe[1] = -1;
    }
  };

  ConnectionState state;
  ConnectionData data;
  URLMatcherData urlMatcherData;
  CGIData cgiData;
  const ServerData *config;

  int client_fd;
  char client_ip[INET_ADDRSTRLEN]; //  remoteAddress;
  bool headers_set; // flag to create the response

  // File handling
  int file_fd;

  // Upload handling
  int writeto_fd;
  char filename[256];
  bool upload_completed;
  size_t bytes_received;

  int errorStatus;
  bool closeConnection;

  HTTPConnxData()
      : state(CONN_INCOMING), data(), client_fd(-1), headers_set(false),
        file_fd(-1), writeto_fd(-1), upload_completed(false), bytes_received(0), 
        config(NULL), errorStatus(0), closeConnection(false) {
    filename[0] = '\0';
    memset(client_ip, 0, sizeof(client_ip));
    config = NULL;  
  }

  void reset(); // will not clear the error status or clientid 
  bool checkHeader(const string &headerName, string &targetVariable);
  string trim(const string &str);
  string formatConnectionData();
  string formatConnectionDataLong();
  string generateSessionId();
  void createSession();
  bool retrieveSession();
  string dechunkData(string chunked_string);
  bool uploadComplete(); 
  bool writingFirstPayloadCompletesUpload();
  bool readFromClientForUpload();
  bool writeUploadToFile();
  bool finishedSendingSimpleResponse();
  void write_to_child_stdin(int current_fd, int pollfd);
  bool settingHeadersIfNeeded(); 
  bool readNewDataFromFile();
  bool sendNewDataFromFileToClient();
  void checkCompletionConditions();
  void read_from_client_into_buffer(); 
  void write_to_client_from_cgi();
  void check_for_client_timeout();
  bool check_for_child_timeout();
  bool sendCgiDataToClient(ssize_t bytes_read);
  void close_conn_after_error();
  bool getDIRListing(string full_path);
  ParseStatus parseRequestLine(const string &line);
  ParseStatus parseHeaderLine(const string &line);
  ParseStatus parseCookies(const string &cookieHeader);
  ParseStatus processContentHeaders();
  ParseStatus parseHeaders();
  ParseStatus extractPortFromHost(std::string &host, uint16_t &port);

};
