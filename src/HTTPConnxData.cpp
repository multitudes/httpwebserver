#include "HTTPConnxData.hpp"
#include "debug.h"
#include <cstring>
#include <sstream>
#include <stdbool.h>
#include <stdlib.h> // for strtoul
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>
#include "Utils.hpp"
#include <signal.h>

using std::map;
using std::string;
using std::vector;


/**
 * @brief Dechunk the data from the string
 * 
 * This function processes the buffer to remove chunked transfer encoding.
 * It will be called once we have the final chunk (0\r\n\r\n).
 * returns the dechunked string
 */
string HTTPConnxData::dechunkData(string chunked_string) {
  std::string dechunked;
  size_t pos = 0;
  
  if (chunked_string.empty() || chunked_string.find("0\r\n\r\n", 0) == string::npos ) {
    debug("ERROR End of chunking not found");
    return dechunked; // Return empty string if end of chunking not found
  }

  while (pos < chunked_string.length()) {
      // Find chunk size line
      size_t chunk_size_end = chunked_string.find("\r\n", pos);
      if (chunk_size_end == std::string::npos) break;
      
      // Parse hex chunk size
      std::string hex_size = chunked_string.substr(pos, chunk_size_end - pos);
      unsigned int chunk_size;
      std::istringstream iss(hex_size);
      iss >> std::hex >> chunk_size;
      
      if (chunk_size == 0) break;  // Last chunk
      
      // Move to chunk data start
      pos = chunk_size_end + 2;
      if (pos + chunk_size > chunked_string.length()) break;
      
      // Append chunk data
      dechunked.append(chunked_string.substr(pos, chunk_size));
      
      // Move to next chunk
      pos += chunk_size + 2;
  }
  
  // Update buffer and headers
  data.headers["Content-Length"] = Utils::to_string(cgiData.buffer.size());
  data.headers.erase("Transfer-Encoding");  // Remove chunked header
  return dechunked;
}

/**
 * @brief Dechunk the data in the buffer
 * 
 * This function processes the buffer to remove chunked transfer encoding.
 * It will be called once we have the final chunk (0\r\n\r\n).
 */
// void HTTPConnxData::dechunkDataCGI() {
//   std::string dechunked;
//   size_t pos = 0;
  
//   while (pos < cgiData.buffer.size()) {
//       // Find chunk size line
//       size_t chunk_size_end = cgiData.buffer.find("\r\n", pos);
//       if (chunk_size_end == std::string::npos) break;
      
//       // Parse hex chunk size
//       std::string hex_size = cgiData.buffer.substr(pos, chunk_size_end - pos);
//       unsigned int chunk_size;
//       std::istringstream iss(hex_size);
//       iss >> std::hex >> chunk_size;
      
//       if (chunk_size == 0) break;  // Last chunk
      
//       // Move to chunk data start
//       pos = chunk_size_end + 2;
//       if (pos + chunk_size > cgiData.buffer.size()) break;
      
//       // Append chunk data
//       dechunked.append(cgiData.buffer.substr(pos, chunk_size));
      
//       // Move to next chunk
//       pos += chunk_size + 2;
//   }
  
//   // Update buffer and headers
//   cgiData.buffer = dechunked;
//   data.headers["Content-Length"] = Utils::to_string(cgiData.buffer.size());
//   data.headers.erase("Transfer-Encoding");  // Remove chunked header
// }



/**
 * @brief Reset the connection for reuse
 *
 * It does NOT close the socket clientfd
 */
void HTTPConnxData::reset() {
  state = CONN_INCOMING;
  data = ConnectionData();
  cgiData = CGIData();
  urlMatcherData = URLMatcherData();
  headers_set = false;
  bytes_received = 0;

  // Close open file descriptors
  if (file_fd != -1) {
    close(file_fd);
    unlink(filename); // Remove partial upload
    file_fd = -1;
    filename[0] = '\0';
  }

  if (writeto_fd != -1) {
    close(writeto_fd);
    writeto_fd = -1;
  }

  //check for cgi and reset
  if (cgiData.cgi_stdin_fd != -1) {
    close(cgiData.cgi_stdin_fd);
    cgiData.cgi_stdin_fd = -1;
  }
  if (cgiData.cgi_stdout_fd != -1) {
    close(cgiData.cgi_stdout_fd);
    cgiData.cgi_stdout_fd = -1;
  }
  if (cgiData.child_pid != -1) {
    ::kill(cgiData.child_pid, SIGTERM);
    cgiData.child_pid = -1;
  }

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

/**
 * @brief Check if a specific header is present and set the target variable
 */
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

/**
 * @brief Parse the request line of the HTTP request
 *
 * It is a util function of the func parseHeaders()
 */
ParseStatus HTTPConnxData::parseRequestLine(const string &line) {
  std::istringstream lineStream(line);
  if (!(lineStream >> data.method >> data.target >> data.version)) {
    debuglog(RED, "Failed to parse request line");
    return HEADERS_PARSE_ERROR;
  }

  // Validate HTTP version
  if (data.version != "HTTP/1.1" && data.version != "HTTP/1.0") {
    debuglog(RED, "Unsupported HTTP version: %s", data.version.c_str());
    debug("Unsupported HTTP version: %s", data.version.c_str());
    return HEADERS_PARSE_ERROR;
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
    return HEADERS_PARSE_ERROR;
  }

  // Parse target into path and query string
  size_t query_pos = data.target.find('?');
  if (query_pos != string::npos) {
    cgiData.query_string = data.target.substr(query_pos + 1);
    debug("Query string: %s", cgiData.query_string.c_str());
    data.target = data.target.substr(0, query_pos);
  } else {
    cgiData.query_string.clear();
  }

  // Find the last dot in the target (file extension)
  size_t last_dot = data.target.find_last_of('.');
  if (last_dot != string::npos) {
    // Find the next slash after the extension
    size_t slash_after_ext = data.target.find('/', last_dot);
    if (slash_after_ext != string::npos) {
      // Everything after the slash is path_info
      cgiData.path_info = data.target.substr(slash_after_ext);
      debug("Path info: %s", cgiData.path_info.c_str());
      // Everything before is the actual target
      data.target = data.target.substr(0, slash_after_ext);
    }
  }

  return HEADERS_PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::parseHeaderLine(const string &line) {
  size_t delimiter = line.find(":");
  if (delimiter == string::npos) {
    debugcolor(RED, "Invalid header line: %s", line.c_str());
    return HEADERS_PARSE_ERROR;
  }

  string key = trim(line.substr(0, delimiter));
  string value = trim(line.substr(delimiter + 1));

  if (key == "Cookie") {
    return parseCookies(value);
  }

  data.headers[key] = value;
  return HEADERS_PARSE_SUCCESS;
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
  return HEADERS_PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::extractPortFromHost(string &host,
                                               uint16_t &port) {
  size_t colon_pos = host.find(':');

  if (colon_pos == string::npos) {
    debuglog(RED, "No port specified in Host header");
    return HEADERS_PARSE_ERROR;
  }

  // Extract port substring
  string port_str = host.substr(colon_pos + 1);
  host = host.substr(0, colon_pos); // Remove port from host string

  // Convert port
  char *endptr;
  long port_long = strtol(port_str.c_str(), &endptr, 10);

  // Validate conversion
  if (*endptr != '\0') {
    debuglog(RED, "Port contains non-numeric characters: %s", port_str.c_str());
    return HEADERS_PARSE_ERROR;
  }

  // Validate range
  if (port_long < 1 || port_long > 65535) { // Port 0 is reserved
    debuglog(RED, "Port out of range (1-65535): %ld", port_long);
    return HEADERS_PARSE_ERROR;
  }

  port = static_cast<uint16_t>(port_long);
  debuglog(YELLOW, "Extracted port: %u", port);
  return HEADERS_PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::processContentHeaders() {
  // Process Host header
  if (!checkHeader(*this, "Host", data.host)) {
    debug("Missing Host header");
    debuglog(RED, "Missing Host header");
    return HEADERS_PARSE_ERROR;
  }

  // Extract port (mandatory )
  if (extractPortFromHost(data.host, data.port) != HEADERS_PARSE_SUCCESS) {
    debug("POrt extraction failed");
    return HEADERS_PARSE_ERROR;
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
      debug("Chunked transfer encoding detected");
      debuglog(YELLOW, "Chunked transfer encoding detected");
    }
  }

  string content_type;
  if (checkHeader(*this, "Content-Type", content_type)) {
    data.headers["Content-Type"] = content_type;

    // Special handling for multipart
    if (content_type.find("multipart/") != string::npos) {
      size_t boundary_pos = content_type.find("boundary=");
      if (boundary_pos == string::npos) {
        debuglog(RED, "No boundary found in multipart form data");
        return HEADERS_PARSE_ERROR;
      }

      boundary_pos += 9; // Skip "boundary="
      data.boundary = "--" + content_type.substr(boundary_pos);
      data.multipart = true;
      data.headers["boundary"] = data.boundary;
    }
  } else {
    // Set default content-type for POST requests
    if (data.method == "POST") {
      data.headers["Content-Type"] = "application/x-www-form-urlencoded";
    }
  }

  // Process Cookies
  string cookieHeader;
  if (checkHeader(*this, "Cookie", cookieHeader)) {
    debuglog(GREEN, "Found cookies in header: %s", cookieHeader.c_str());

    // Split cookies by semicolon
    std::istringstream cookieStream(cookieHeader);
    string cookiePair;

    while (std::getline(cookieStream, cookiePair, ';')) {
      // Trim whitespace
      size_t start = cookiePair.find_first_not_of(" \t");
      if (start == string::npos)
        continue;
      cookiePair = cookiePair.substr(start);

      // Split by equals sign
      size_t equalPos = cookiePair.find('=');
      if (equalPos != string::npos) {
        string name = cookiePair.substr(0, equalPos);
        string value = cookiePair.substr(equalPos + 1);

        // Store the cookie
        data.cookies[name] = value;
        debuglog(GREEN, "Parsed cookie: %s = %s", name.c_str(), value.c_str());
      }
    }
  }

  return HEADERS_PARSE_SUCCESS;
}

ParseStatus HTTPConnxData::parseHeaders(HTTPConnxData &conn) {
  ConnectionData &data = conn.data;
  if (data.request.empty()) {
    debug("Empty request received");
    return HEADERS_PARSE_INCOMPLETE;
  }

  data.headers_end = data.request.find("\r\n\r\n");
  if (data.headers_end == string::npos) {
    debug("Headers not complete");
    return HEADERS_PARSE_INCOMPLETE;
  }

  data.headers_end += 4; // Skip \r\n\r\n
  data.headers_received = true;
  debug("Headers complete");
  std::istringstream iss(data.request);
  string line;

  // Parse request line
  if (!std::getline(iss, line) || parseRequestLine(line) != HEADERS_PARSE_SUCCESS) {
    return HEADERS_PARSE_ERROR;
  }

  // Parse headers
  while (std::getline(iss, line)) {
    if (line.empty() || line == "\r")
      break;
    if (parseHeaderLine(line) != HEADERS_PARSE_SUCCESS) {
      return HEADERS_PARSE_ERROR;
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
      << (data.headers_set ? " HDRS_SENT" : "")
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
    for (map<string, string>::const_iterator it =
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
    for (map<string, string>::const_iterator it =
             data.cookies.begin();
         it != data.cookies.end() && count < 2; ++it, ++count) {
      if (count > 0)
        oss << ", ";
      oss << "\"" << it->first << "\":\"" << it->second << "\"";
    }
    if (data.cookies.size() > 2) {
      oss << ", ... (" << data.cookies.size() - 2 << " more)";
    }
    oss << "]";
  }

  // Response info
  oss << ", response_status=" << data.response_status;
  oss << ", bytes_sent=" << data.bytes_sent;
  oss << ", headers_set=" << (data.headers_set ? "true" : "false");
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

// SESSION MANAGEMENT FUNCTIONS---------------------------------Rufus

// Generate a unique session ID using timestamp
string HTTPConnxData::generateSessionId() {
  // Get current time
  time_t now = time(NULL);
  // Convert to hex string with padding
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << now;
  // Add process ID for additional uniqueness
  ss << "_" << std::hex << getpid();
  debuglog(YELLOW, "Generated session ID: %s", ss.str().c_str());
  return ss.str();
}

// Create a new session for the current connection
void HTTPConnxData::createSession() {
  data.session_id = generateSessionId();
  data.has_session = true;
  data.session_created = time(NULL);
  data.session_last_accessed = time(NULL);
  // Reset any previous session data
  data.session_data.clear();
  // Add session cookie to response headers
  string cookie =
      "Set-Cookie: sessionid=" + data.session_id + "; Path=/; HttpOnly\r\n";
  data.response_headers += cookie;
}

// Try to retrieve session from cookies
bool HTTPConnxData::retrieveSession() {
  // Check if we already have a session for this connection
  if (data.has_session && !data.session_id.empty()) {
    debuglog(GREEN, "Session already loaded: %s", data.session_id.c_str());
    return true;
  }

  // Check if a sessionid cookie exists
  if (data.cookies.find("sessionid") != data.cookies.end()) {
    data.session_id = data.cookies["sessionid"];
    data.has_session = true;

    // Check if the session has expired
    time_t now = time(NULL);
    time_t sessionExpiry = data.session_last_accessed + 30; // 30 seconds expiry
    if (now > sessionExpiry) {
      debuglog(RED, "Session expired. Clearing session.");
      data.has_session = false;
      data.session_id.clear();
      data.session_data.clear();
      return false; // Session expired
    }

    // Update session_last_accessed
    data.session_last_accessed = now;
    debuglog(GREEN, "Session found in cookies: %s", data.session_id.c_str());
    return true;
  }

  debuglog(YELLOW, "No session cookie found");
  return false;
}
// end SESSION MANAGEMENT FUNCTIONS---------------------------------Rufus