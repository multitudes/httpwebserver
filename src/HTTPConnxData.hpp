#pragma once

#include <map>
#include <string>
#include <sys/types.h>
#include <vector>
#include <unistd.h>

using std::map;
using std::string;
using std::vector;

/**
 * @brief Connection state enum
 *
 * Tracks the current state of a connection throughout its lifecycle
 */
enum ConnectionState {
    CONN_INCOMING,      // New connection, nothing processed yet
    CONN_PARSING_HEADER,// Receiving/parsing headers
    CONN_CGI,           // Processing CGI request
    CONN_FILE_REQUEST,  // Serving a file
    CONN_UPLOAD,        // Handling file upload
    CONN_CLOSING        // Ready to close
};

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
        string method;
        string target;
        string version;
        string host;
        string request;
        size_t content_length;
        map<std::string, std::string> headers;
        map<std::string, std::string> cookies;
        bool headers_received;
        bool is_get_request;
        bool chunked;
        bool multipart;
        string boundary;
        size_t headers_end;
        bool sending_response;
        string response;
        size_t bytes_sent;
        bool response_sent;

        ConnectionData() :
            method(""), target(""), version(""), host(""), request(""),
            content_length(0), headers(), cookies(),
            headers_received(false), is_get_request(false),
            chunked(false), multipart(false), boundary(""),
            headers_end(0), sending_response(false),
            response(""), bytes_sent(0), response_sent(false) {}
    };

    // Connection state and metadata
    ConnectionState state;
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
    bool is_uploading;
    size_t bytes_received;

    // Request data
    ConnectionData data;

    HTTPConnxData() :
        state(CONN_INCOMING),
        client_fd(-1),
        indexServerConf(-1),
        is_sending(0),
        is_receiving(0),
        headers_sent(false),
        poll_stdin_idx(-1),
        poll_stdout_idx(-1),
        child_pid(-1),
        cgi_processing(false),
        file_fd(-1),
        file_size(0),
        file_offset(0),
        writeto_fd(-1),
        is_uploading(false),
        bytes_received(0),
        data() {
        filename[0] = '\0';
    }

    void reset();
	bool checkHeader(HTTPConnxData& state,
		const string& headerName,
		string& targetVariable);
	string trim(const string& str);
	bool parsingHeaders(int client_fd, HTTPConnxData& state);
};