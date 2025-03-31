#include "HTTPConnxData.hpp"
#include "debug.h"
#include <sstream>
#include <stdbool.h>
#include <stdlib.h> // for strtoul
#include <string>
#include <unistd.h>

using std::map;
using std::string;
using std::vector;

// TODO refactor - remove with new
//  bool HTTPConnxData::parsingHeaders(int client_fd, HTTPConnxData &connx) {
//    if (connx.data.request.empty()) {
//      debuglog(YELLOW, "Empty request received in parseHeaders");
//      return false;
//    }
//    connx.data.headers_end = connx.data.request.find("\r\n\r\n");
//    // found the "\r\n\r\n" sequence
//    if (connx.data.headers_end != string::npos) {
//      debuglog(YELLOW, "Received headers");
//      connx.data.headers_received = true;
//      connx.data.headers_end += 4; // skip the \r\n\r\n

//     // parse line by line and add to headers map
//     std::istringstream iss(connx.data.request);
//     string line;
//     // parse the first request line, e.g. "GET /index.html HTTP/1.1"
//     if (std::getline(iss, line)) {
//       std::istringstream lineStream(line);
//       if (!(lineStream >> connx.data.method >> connx.data.target >>
//             connx.data.version)) {
//         debuglog(RED, "failed to parse headers request line");
//         return false;
//       }
//       // validation.
//       // if (state.data.headers["version"] != "HTTP/1.1" &&
//       // state.data.headers["version"] != "HTTP/1.0") { 	debuglog(RED,
//       // "Unsupported HTTP version"); 	return
//       // false;
//       // }
//     } else {
//       debuglog(RED, "Empty request");
//       return false;
//     }
//     // parse the rest
//     while (std::getline(iss, line)) {
//       // end of headers
//       if (line.empty() || line == "\r") {
//         break;
//       }
//       size_t delimiter = line.find(":");
//       if (delimiter != string::npos) {
//         string key = trim(line.substr(0, delimiter));
//         string value = trim(line.substr(delimiter + 1));
//         if (key == "Cookie") {
//           // Parse cookies
//           std::istringstream cookieStream(value);
//           string cookiePair;
//           while (std::getline(cookieStream, cookiePair, ';')) {
//             cookiePair = trim(cookiePair);
//             size_t cookieDelimiter = cookiePair.find("=");
//             if (cookieDelimiter != string::npos) {
//               string cookieName = trim(cookiePair.substr(0,
//               cookieDelimiter)); string cookieValue =
//               trim(cookiePair.substr(cookieDelimiter + 1));
//               connx.data.cookies[cookieName] = cookieValue;
//             }
//           }
//         } else {
//           connx.data.headers.insert(std::make_pair(key, value));
//         }
//       } else {
//         debugcolor(RED, "Invalid header line: %s", line.c_str());
//         return false;
//       }
//     }
//     // host is mandatory
//     if (checkHeader(connx, "Host", connx.data.host)) {
//       debuglog(YELLOW, "Host: %s", connx.data.host.c_str());
//     } else {
//       return false;
//     }
//     string content_length_str;
//     if (checkHeader(connx, "Content-Length", content_length_str)) {
// 		connx.data.content_length =
// 			strtoul(content_length_str.c_str(), NULL, 10);
//       debuglog(YELLOW, "Content-Length: %ld", connx.data.content_length);
//     } else {
//       debuglog(YELLOW, "No Content-Length header found");
//       // check for chunked encoding
//       string transfer_encoding;
//       string multipart;
//       if (checkHeader(connx, "Transfer-Encoding", transfer_encoding)) {
//         if (transfer_encoding == "chunked") {
//           debuglog(YELLOW, "Chunked transfer encoding detected");
//           connx.data.chunked = true;
//         }
//       } else if (connx.data.request.find("Content-Type: multipart/") !=
//                  string::npos) {
//         size_t boundary_pos = connx.data.request.find("boundary=");
//         if (boundary_pos != string::npos) {
//           connx.data.multipart = true;
//           boundary_pos += 9; // skip the "boundary="
//           size_t boundary_end = connx.data.request.find("\r\n",
//           boundary_pos); connx.data.boundary =
//               "--" + connx.data.request.substr(boundary_pos,
//                                                boundary_end - boundary_pos);
//           debuglog(YELLOW, "Boundary: %s\n", connx.data.boundary.c_str());
//           connx.data.headers["Content-Type"] = "multipart/form-data";
//           connx.data.headers["boundary"] = connx.data.boundary;
//         } else {
//           debuglog(RED, "No boundary found - invalid multipart form data");
//           return false;
//         }
//       } else if (connx.data.request.substr(0, 3) == "GET" ||
//                  connx.data.request.substr(0, 6) == "DELETE") {
//         debug("Request complete\n");
//         connx.data.is_get_request = true;
//         connx.data.headers_received = true;
//       }
//     }
//   }
//   return true;
// }

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

bool HTTPConnxData::checkHeader(HTTPConnxData &state, const string &headerName,
                                string &targetVariable) {
  map<string, string>::iterator headerIt = state.data.headers.find(headerName);
  if (headerIt != state.data.headers.end()) {
    targetVariable = headerIt->second;
    return true; // Header found and set
  } else {
    return false; // Header not found
  }
}

ParseStatus HTTPConnxData::parseRequestLine(const string &line) {
  std::istringstream lineStream(line);
  if (!(lineStream >> data.method >> data.target >> data.version)) {
    debuglog(RED, "Failed to parse request line");
    return PARSE_ERROR;
  }

  // Validate HTTP version
  if (data.version != "HTTP/1.1" && data.version != "HTTP/1.0") {
    debuglog(RED, "Unsupported HTTP version: %s", data.version.c_str());
    return PARSE_ERROR;
  }

  // Validate method
  const char *methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD"};
  bool valid = false;
  for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
    if (data.method == methods[i]) {
      valid = true;
      break;
    }
  }
  if (!valid) {
    debuglog(RED, "Invalid HTTP method: %s", data.method.c_str());
    return PARSE_ERROR;
  }

  data.is_get_request = (data.method == "GET");
  return PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::parseHeaderLine(const string &line) {
  size_t delimiter = line.find(":");
  if (delimiter == string::npos) {
    debugcolor(RED, "Invalid header line: %s", line.c_str());
    return PARSE_ERROR;
  }

  string key = trim(line.substr(0, delimiter));
  string value = trim(line.substr(delimiter + 1));

  if (key == "Cookie") {
    return parseCookies(value);
  }

  data.headers[key] = value;
  return PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::parseCookies(const string &cookieHeader) {
  std::istringstream cookieStream(cookieHeader);
  string cookiePair;

  while (std::getline(cookieStream, cookiePair, ';')) {
    cookiePair = trim(cookiePair);
    size_t cookieDelimiter = cookiePair.find("=");
    if (cookieDelimiter == string::npos)
      continue;

    string cookieName = trim(cookiePair.substr(0, cookieDelimiter));
    string cookieValue = trim(cookiePair.substr(cookieDelimiter + 1));
    data.cookies[cookieName] = cookieValue;
  }
  return PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::processContentHeaders() {
  // Process Host header
  if (!checkHeader(*this, "Host", data.host)) {
    debuglog(RED, "Missing Host header");
    return PARSE_ERROR;
  }

  // Process Content-Length
  string content_length_str;
  if (checkHeader(*this, "Content-Length", content_length_str)) {
    data.content_length = strtoul(content_length_str.c_str(), NULL, 10);
    debuglog(YELLOW, "Content-Length: %ld", data.content_length);
  }

  // Process Transfer-Encoding
  string transfer_encoding;
  if (checkHeader(*this, "Transfer-Encoding", transfer_encoding)) {
    data.chunked = (transfer_encoding == "chunked");
    if (data.chunked) {
      debuglog(YELLOW, "Chunked transfer encoding detected");
    }
  }

  // Process Multipart
  if (data.request.find("Content-Type: multipart/") != string::npos) {
    size_t boundary_pos = data.request.find("boundary=");
    if (boundary_pos == string::npos) {
      debuglog(RED, "No boundary found in multipart form data");
      return PARSE_ERROR;
    }

    boundary_pos += 9; // Skip "boundary="
    size_t boundary_end = data.request.find("\r\n", boundary_pos);
    data.boundary =
        "--" + data.request.substr(boundary_pos, boundary_end - boundary_pos);
    data.multipart = true;
    data.headers["Content-Type"] = "multipart/form-data";
    data.headers["boundary"] = data.boundary;
  }

  return PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::parseHeaders(HTTPConnxData &conn) {
  ConnectionData &data = conn.data;
  if (data.request.empty()) {
    debuglog(YELLOW, "Empty request received");
    return PARSE_INCOMPLETE;
  }

  data.headers_end = data.request.find("\r\n\r\n");
  if (data.headers_end == string::npos) {
    return PARSE_INCOMPLETE;
  }

  data.headers_end += 4; // Skip \r\n\r\n
  data.headers_received = true;

  std::istringstream iss(data.request);
  string line;

  // Parse request line
  if (!std::getline(iss, line) || parseRequestLine(line) != PARSE_SUCCESS) {
    return PARSE_ERROR;
  }

  // Parse headers
  while (std::getline(iss, line)) {
    if (line.empty() || line == "\r")
      break;
    if (parseHeaderLine(line) != PARSE_SUCCESS) {
      return PARSE_ERROR;
    }
  }

  // Process content-related headers
  return processContentHeaders();
}

/**
 * @brief Truncate a string to a maximum length and append the length
 */
string trunc(const string s) {
  size_t max = 32;
  if (s.length() <= max)
    return s;
  char buf[64];
  snprintf(buf, sizeof(buf), "%.*s...[%lu]", (int)max, s.c_str(),
           (unsigned long)s.length());
  return buf;
}

/**
 * @brief Format the connection data for logging
 *
 * I asked deepseek for a pretty printing of the connection data for
 * debugging purposes.
 */
string HTTPConnxData::formatConnectionData(const ConnectionData &data) {
  std::ostringstream oss;

  // Core request info
  oss << "ConnectionData{"
      << "method=\"" << data.method << "\" "
      << "target=\"" << data.target << "\" "
      << "version=\"" << data.version << "\" "
      << "host=\"" << data.host << "\""
      << ":" << data.port;

  // Body metadata
  oss << " cl=" << data.content_length << (data.chunked ? " chunked" : "")
      << (data.multipart ? " multipart" : "");

  // Request snippet
  if (!data.request.empty()) {
    oss << " req=\"" << trunc(data.request) << "\"";
  }

  // Compact headers/cookies count
  oss << " hdrs=" << data.headers.size() << " cookies=" << data.cookies.size();

  // Response state
  oss << " status=" << data.response_status << " sent=" << data.bytes_sent
      << "/" << (data.response_body.empty() ? 0 : data.response_body.length());

  // Flags at the end
  oss << (data.headers_received ? " HDRS_RCVD" : "")
      << (data.headers_sent ? " HDRS_SENT" : "")
      << (data.response_sent ? " RESP_SENT" : "");

  oss << "}";
  return oss.str();
}

/**
 * @brief Format the connection data for logging - long version
 */
string HTTPConnxData::formatConnectionDataLong(const ConnectionData &data) {
  std::ostringstream oss;

  oss << "ConnectionData { "
      << "method=\"" << data.method << "\", "
      << "target=\"" << data.target << "\", "
      << "version=\"" << data.version << "\", "
      << "host=\"" << data.host << "\", "
      << "port=" << data.port << ", "
      << "content_length=" << data.content_length << ", "
      << "headers_received=" << (data.headers_received ? "true" : "false")
      << ", "
      << "is_get_request=" << (data.is_get_request ? "true" : "false") << ", "
      << "chunked=" << (data.chunked ? "true" : "false") << ", "
      << "multipart=" << (data.multipart ? "true" : "false");

  if (!data.boundary.empty()) {
    oss << ", boundary=\"" << data.boundary << "\"";
  }

  // Print headers count
  oss << ", headers_count=" << data.headers.size();

  // Print first few headers if available
  if (!data.headers.empty()) {
    oss << ", headers=[";
    size_t count = 0;
    for (std::map<std::string, std::string>::const_iterator it =
             data.headers.begin();
         it != data.headers.end() && count < 3; ++it, ++count) {
      if (count > 0)
        oss << ", ";
      oss << "\"" << it->first << "\":\"" << it->second << "\"";
    }
    if (data.headers.size() > 3) {
      oss << ", ... (" << (data.headers.size() - 3) << " more)";
    }
    oss << "]";
  }

  // Print cookies count
  oss << ", cookies_count=" << data.cookies.size();

  // Print first few cookies if available
  if (!data.cookies.empty()) {
    oss << ", cookies=[";
    size_t count = 0;
    for (std::map<std::string, std::string>::const_iterator it =
             data.cookies.begin();
         it != data.cookies.end() && count < 2; ++it, ++count) {
      if (count > 0)
        oss << ", ";
      oss << "\"" << it->first << "\":\"" << it->second << "\"";
    }
    if (data.cookies.size() > 2) {
      oss << ", ... (" << (data.cookies.size() - 2) << " more)";
    }
    oss << "]";
  }

  // Response info
  oss << ", response_status=" << data.response_status;
  oss << ", bytes_sent=" << data.bytes_sent;
  oss << ", headers_sent=" << (data.headers_sent ? "true" : "false");
  oss << ", sending_response=" << (data.sending_response ? "true" : "false");
  oss << ", response_sent=" << (data.response_sent ? "true" : "false");

  // Truncate request/response if too long
  const size_t MAX_DISPLAY_LENGTH = 50;
  if (!data.request.empty()) {
    oss << ", request=\"";
    if (data.request.length() > MAX_DISPLAY_LENGTH) {
      oss << data.request.substr(0, MAX_DISPLAY_LENGTH) << "...\" ("
          << data.request.length() << " chars)";
    } else {
      oss << data.request << "\"";
    }
  }

  if (!data.response.empty()) {
    oss << ", response=\"";
    if (data.response.length() > MAX_DISPLAY_LENGTH) {
      oss << data.response.substr(0, MAX_DISPLAY_LENGTH) << "...\" ("
          << data.response.length() << " chars)";
    } else {
      oss << data.response << "\"";
    }
  }

  oss << " }";

  return oss.str();
}