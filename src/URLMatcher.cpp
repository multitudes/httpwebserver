#include "URLMatcher.hpp"
#include "CGI.hpp"
#include "DirectoryListing.hpp"
#include "HTTPConnxData.hpp"
#include "HTTPServer.hpp"
#include "debug.h"
#include <map>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include "SimpleResponse.hpp" // Ensure this header is included

#define BUFFER_SIZE 4096

namespace URLMatcher {

void validateRequest(HTTPConnxData &conn) {
  debuglog(YELLOW, "Connection on fd %d in state INCOMING", conn.client_fd);
  // check if the headers are received
  // if not set to CONN_PARSING_HEADER
  char buffer[BUFFER_SIZE];
  int bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);

  if (bytes_read <= 0) 
  {
    if (bytes_read == 0) {
      debug("Client disconnected");
	  HTTPServer::remove_from_poll(conn.client_fd);
	  close(conn.client_fd);
	  conn.reset();
	  	
    } else {
      perror("recv failed");
    }
    conn.reset();
    return;
  }

  debuglog(YELLOW, "Received %d bytes from client\n", bytes_read);

  conn.data.request.append(buffer, bytes_read);
  if (!conn.parsingHeaders(conn.client_fd, conn)) {
    HTTPServer::remove_from_poll(conn.client_fd);
    conn.reset();
    return;
  }
  // debugcolor(PASTEL_MAGENTA,"Headers received \n%s",
  // conn.data.request.c_str());
  if (!conn.data.headers_received) {
    return;
  }


    /*
    ths code to check if directory or file if internal if file listing
    should be here not in getdir listing
    */
   

  // check if the request contain cgi for debug now
  // if so set to CONN_CGI - hardcoded for now
  // TODO check if the request is a cgi request
  if (conn.data.request.find("cgi") != string::npos) 
  {
    conn.state = CONN_CGI;
    debug("CGI request detected");
    // Start CGI process for this connection
    if (CGI::prepareCGI(conn) < 0) {
      conn.reset();
      return;
    }
    // hardcoded for now ---
  } else if (conn.data.request.find("upload") != string::npos) {
    conn.state = CONN_UPLOAD;
    debuglog(YELLOW, "Upload request detected");
    // TODO prepare fd for upload

    // First chunk - extract filename and open file
    HTTPServer::extract_filename(buffer, conn.filename);

    conn.file_fd = open(conn.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (conn.file_fd < 0) {
      perror("Failed to open file for upload");
      conn.reset();
      return;
    }
    debuglog(YELLOW, "Starting upload to: %s\n", conn.filename);
  } else {
    std::string full_path; // holds the GET target path


    std::string root = Config::getConfigByPort(4244)->root; // TODO hardcoded for now: we need to add the port from the config
    std::string target = conn.data.target;
    if (target[0] == '/') { // checks for a leading '/' and skips it
      target = target.substr(1);
    }
    // TODO check if index.html or other default file from config exists, if so return it
    full_path = root + "/" + target;
    debuglog(YELLOW, "Full path: %s", full_path.c_str());
    
    conn.state = CONN_FILE_REQUEST;
    debuglog(YELLOW, "File request detected");

    if (DirectoryListing::getDIRListing(conn, full_path)) 
    {
      conn.state = CONN_SIMPLE_RESPONSE;
      HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
      return;
    } 
    else {
        conn.file_fd = open(full_path.c_str(), O_RDONLY);
        if (conn.file_fd < 0) {
          perror("Failed to open file");
          
          SimpleResponse::htmlErrorResponse(conn, 404);
          HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
          return;
        }

      struct stat file_stat;
      if (fstat(conn.file_fd, &file_stat) < 0) {
        perror("Failed to get file stats");
        close(conn.file_fd);
        conn.state = CONN_SIMPLE_RESPONSE;
        SimpleResponse::htmlErrorResponse(conn, 404);
        HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
        // TODO prepare error response
        return;
      }

      conn.file_size = file_stat.st_size;
      debuglog(YELLOW, "Opening file index.html for fd %d", conn.file_fd);
    }

    HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
    debuglog(YELLOW, "Sending response to fd %d", conn.client_fd);
  }
}

} // namespace URLMatcher