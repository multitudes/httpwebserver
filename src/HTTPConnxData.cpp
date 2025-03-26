#include "HTTPConnxData.hpp"
#include "debug.h"
#include <sstream>
#include <stdbool.h>
#include <string>
#include <unistd.h>
#include <stdlib.h> // for strtoul

bool HTTPConnxData::parsingHeaders(int client_fd, HTTPConnxData &state) {
  if (state.data.request.empty()) {
    debuglog(YELLOW, "Empty request received in parseHeaders");
    return false;
  }
  state.data.headers_end = state.data.request.find("\r\n\r\n");
  // found the "\r\n\r\n" sequence
  if (state.data.headers_end != string::npos) {
    debuglog(YELLOW, "Received headers");
    state.data.headers_received = true;
    state.data.headers_end += 4; // skip the \r\n\r\n

    // parse line by line and add to headers map
    std::istringstream iss(state.data.request);
    string line;
    // parse the first request line, e.g. "GET /index.html HTTP/1.1"
    if (std::getline(iss, line)) {
      std::istringstream lineStream(line);
      if (!(lineStream >> state.data.method >> state.data.target >>
            state.data.version)) {
        debuglog(RED, "failed to parse headers request line");
        return false;
      }
      // validation.
      // if (state.data.headers["version"] != "HTTP/1.1" &&
      // state.data.headers["version"] != "HTTP/1.0") { 	debuglog(RED,
      // "Unsupported HTTP version"); 	return
      // false;
      // }
    } else {
      debuglog(RED, "Empty request");
      return false;
    }
    // parse the rest
    while (std::getline(iss, line)) {
      // end of headers
      if (line.empty() || line == "\r") {
        break;
      }
      size_t delimiter = line.find(":");
      if (delimiter != string::npos) {
        string key = trim(line.substr(0, delimiter));
        string value = trim(line.substr(delimiter + 1));
        if (key == "Cookie") {
          // Parse cookies
          std::istringstream cookieStream(value);
          string cookiePair;
          while (std::getline(cookieStream, cookiePair, ';')) {
            cookiePair = trim(cookiePair);
            size_t cookieDelimiter = cookiePair.find("=");
            if (cookieDelimiter != string::npos) {
              string cookieName = trim(cookiePair.substr(0, cookieDelimiter));
              string cookieValue = trim(cookiePair.substr(cookieDelimiter + 1));
              state.data.cookies[cookieName] = cookieValue;
            }
          }
        } else {
          state.data.headers.insert(std::make_pair(key, value));
        }
      } else {
        debugcolor(RED, "Invalid header line: %s", line.c_str());
        return false;
      }
    }
    // host is mandatory
    if (checkHeader(state, "Host", state.data.host)) {
      debuglog(YELLOW, "Host: %s", state.data.host.c_str());
    } else {
      return false;
    }
    string content_length_str;
    if (checkHeader(state, "Content-Length", content_length_str)) {
		state.data.content_length =
			strtoul(content_length_str.c_str(), NULL, 10);
      debuglog(YELLOW, "Content-Length: %ld", state.data.content_length);
    } else {
      debuglog(YELLOW, "No Content-Length header found");
      // check for chunked encoding
      string transfer_encoding;
      string multipart;
      if (checkHeader(state, "Transfer-Encoding", transfer_encoding)) {
        if (transfer_encoding == "chunked") {
          debuglog(YELLOW, "Chunked transfer encoding detected");
          state.data.chunked = true;
        }
      } else if (state.data.request.find("Content-Type: multipart/") !=
                 string::npos) {
        size_t boundary_pos = state.data.request.find("boundary=");
        if (boundary_pos != string::npos) {
          state.data.multipart = true;
          boundary_pos += 9; // skip the "boundary="
          size_t boundary_end = state.data.request.find("\r\n", boundary_pos);
          state.data.boundary =
              "--" + state.data.request.substr(boundary_pos,
                                               boundary_end - boundary_pos);
          debuglog(YELLOW, "Boundary: %s\n", state.data.boundary.c_str());
          state.data.headers["Content-Type"] = "multipart/form-data";
          state.data.headers["boundary"] = state.data.boundary;
        } else {
          debuglog(RED, "No boundary found - invalid multipart form data");
          return false;
        }
      } else if (state.data.request.substr(0, 3) == "GET" ||
                 state.data.request.substr(0, 6) == "DELETE") {
        debug("Request complete\n");
        state.data.is_get_request = true;
        state.data.headers_received = true;
      }
    }
  }
  return true;
}

/**
 * @brief Reset the connection for reuse
 */
void HTTPConnxData::reset() {
  state = CONN_INCOMING;
  is_sending = 0;
  is_receiving = 0;
  headers_sent = false;
  cgi_processing = false;
  bytes_received = 0;
  data = ConnectionData();

  // Close open file descriptors
  if (file_fd != -1)
    close(file_fd);
  if (writeto_fd != -1)
    close(writeto_fd);

  file_fd = -1;
  writeto_fd = -1;
  filename[0] = '\0';
}

/**
 * @brief Trim leading and trailing whitespace from a string
 *
 * @param str The string to trim
 * @return The trimmed string
 */
string HTTPConnxData::trim(const string &str) {
  string trimmed = str;
  string whitespaces = " \r\n\t";
  size_t start = trimmed.find_first_not_of(whitespaces);
  if (start == string::npos) {
    return "";
  }
  size_t end = trimmed.find_last_not_of(whitespaces);
  return trimmed.substr(start, end - start + 1);
}

bool HTTPConnxData::checkHeader(HTTPConnxData &state,
                                 const string &headerName,
                                 string &targetVariable) {
  map<string, string>::iterator headerIt = state.data.headers.find(headerName);
  if (headerIt != state.data.headers.end()) {
    targetVariable = headerIt->second;
    return true; // Header found and set
  } else {
    return false; // Header not found
  }
}