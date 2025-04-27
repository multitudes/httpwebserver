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
#include "SocketUtils.hpp"
#include "HTTPServer.hpp"
#include "Constants.hpp"
#include <cassert>
#include <dirent.h> 
#include "Responses.hpp"

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
 * @brief Read data from the client into the buffer
 *
 * @param conn The connection data
 * @param current_fd The current file descriptor
 */
void HTTPConnxData::read_from_client_into_buffer() {
  cgiData.buffer.resize(Constants::BUFFER_SIZE);
  ssize_t bytes_read =
      ::recv(client_fd, &cgiData.buffer[0], Constants::BUFFER_SIZE, 0);
  if (bytes_read < 0) {
    perror("Failed to read from client");
    state = CONN_CGI_FINISHED;
  } else if (bytes_read == 0) {
    debug("Client closed connection - giving EOF to CGI stdin");
    close(cgiData.cgi_stdin_fd);
    SocketUtils::remove_from_poll(cgiData.cgi_stdin_fd);
    cgiData.cgi_stdin_fd = -1; // Mark as closed
    state = CONN_CGI_SENDING;
  }
  debug("Received %ld bytes from client", bytes_read);
}

/**
 * @brief Write the buffer to the client from CGI
 *
 * @param conn The connection data
 * @param current_fd The current file descriptor
 */
void HTTPConnxData::write_to_client_from_cgi() {
  if (!cgiData.buffer.empty()) {
    debug("leftover buffer from cgiData");
    // write to cgi the buffer if any remaining from the
    // initialisation
    ssize_t bytes_written = ::send(client_fd, cgiData.buffer.c_str(),
                                    cgiData.buffer.size(), MSG_NOSIGNAL);
    debug("Wrote %ld bytes to client", bytes_written);

    if (bytes_written < 0) {
      perror("Failed to write to client");
      debug("Failed to write to client");
      state = CONN_CGI_FINISHED;
    } else if (bytes_written == 0) {
      // Should not happen with blocking write unless size was 0
      debuglog(YELLOW, "Wrote 0 bytes to client (buffer size: %zu)",
               cgiData.buffer.size());
      debuglog(RED, "Wrote 0 bytes to client unexpectedly.");
      state = CONN_CGI_FINISHED;
    } else if (static_cast<size_t>(bytes_written) <
               cgiData.buffer.size()) {
      // Partial write: Remove written data and wait for next POLLOUT
      debug("Partial write: Wrote %ld bytes to client (buffer size: %zu)",
            bytes_written, cgiData.buffer.size());
      cgiData.buffer.erase(
          0, static_cast<std::string::size_type>(bytes_written));
      // stay in the same state, poll will trigger again
    } else {
      // Full write (bytes_written == cgiData.buffer.size())
      debugcolor(MAGENTA, "wrote request buffer to client: %s",
                 cgiData.buffer.c_str()); // Log data before clearing
      // TODO check the bytes received
      cgiData.bytes_received += static_cast<size_t>(bytes_written);
      if (cgiData.bytes_received >= data.content_length) {
        debug("Full write: Wrote %ld bytes to CGI stdin", bytes_written);
        // If we have written all data, clear the buffer
        cgiData.buffer.clear();
      }
    }
  }
}

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
bool HTTPConnxData::checkHeader(const string &headerName,
                                string &targetVariable) {
  map<string, string>::iterator headerIt = data.headers.find(headerName);
  if (headerIt != data.headers.end()) {
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

/**
 * @brief Parse a single header line
 *
 * @param line The header line to parse
 * @return ParseStatus indicating success or failure
 * 
 * This is assuming that the header line is in the format "Key: Value"
 */
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

/**
 * @brief Parse cookies from the Cookie header
 *
 * @param cookieHeader The Cookie header string
 * @return ParseStatus indicating success or failure
 */
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

/**
 * * @brief Extract the port from the Host header
 */
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
  long port_long = ::strtol(port_str.c_str(), &endptr, 10);
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

/**
 * @brief Process content-related headers
 */
ParseStatus HTTPConnxData::processContentHeaders() {
  // Process Host header
  if (!checkHeader("Host", data.host)) {
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
  if (checkHeader("Content-Length", content_length_str)) {
    data.content_length = strtoul(content_length_str.c_str(), NULL, 10);
    debuglog(YELLOW, "Content-Length: %ld", data.content_length);
  }

  // Process Transfer-Encoding
  string transfer_encoding;
  if (checkHeader("Transfer-Encoding", transfer_encoding)) {
    data.chunked = (transfer_encoding == "chunked");
    if (data.chunked) {
      debug("Chunked transfer encoding detected");
      debuglog(YELLOW, "Chunked transfer encoding detected");
    }
  }

  string content_type;
  if (checkHeader("Content-Type", content_type)) {
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
  if (checkHeader("Cookie", cookieHeader)) {
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

/**
 * * @brief Parse the headers of the HTTP request
 */
ParseStatus HTTPConnxData::parseHeaders() {
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
string HTTPConnxData::formatConnectionData() {
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
  oss << " status=" << data.response_status << " sent=" << data.bytes_sent;

  // Flags at the end
  oss << (data.headers_received ? " HDRS_RCVD" : "")
      << (data.response_sent ? " RESP_SENT" : "");

  oss << "}";
  return oss.str();
}

/**
 * @brief Format the connection data for logging - long version
 */
string HTTPConnxData::formatConnectionDataLong() {
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

/**
 * @brief Generate a unique session ID using timestamp and process ID
 */
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

/**
 * * @brief Create a new session for the current connection
 */
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

/**
 * @brief Try to retrieve session from cookies
 */
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

/**
 * @brief Check if the upload is complete
 *
 * @param conn The connection data
 * @return true if the upload is complete, false otherwise
 *
 * This function checks if the number of bytes sent is greater than or equal to
 * the content length. If so, it resets the connection and sends a response to
 * the client.
 */
bool HTTPConnxData::uploadComplete() {
  if (data.bytes_sent >= data.content_length) {
    debug("Upload complete");
    reset();
    Responses::createResponse(*this, "text/plain", "File uploaded successfully.",
                              201);
    state = CONN_SIMPLE_RESPONSE;
    return true;
  }
  return false;
}

/**
 * @brief Check if writing the first payload completes the upload
 *
 * @param conn The connection data
 * @return true if the upload is complete, false otherwise
 *
 * This function checks if there is any leftover payload data from the header
 * parsing. If so, it writes that data to the file and checks if the upload is
 * complete.
 */
bool HTTPConnxData::writingFirstPayloadCompletesUpload() {
  if (!data.response.empty()) {
    debug("Writing leftover payload for connection %d", client_fd);
    debuglog(YELLOW, "writing leftover payload for client %d", client_fd);
    ssize_t bytes_written = write(file_fd, data.response.c_str(),
                                  data.response.size());
    if (bytes_written <= 0) {
      perror(bytes_written < 0 ? "Failed to write to file"
                               : "No data written to file");
      reset();
      close(client_fd);
      SocketUtils::remove_from_poll(client_fd);
      client_fd = -1; // Mark as closed
      return true;
    }
    data.bytes_sent += static_cast<size_t>(bytes_written);
    data.response.clear();

    if (uploadComplete()) {
      return true;
      ;
    }
  }
  return false;
}

/**
 * @brief Read data from the client for upload
 */
bool HTTPConnxData::readFromClientForUpload() {
  data.buffer.resize(Constants::BUFFER_SIZE);
  ssize_t bytes_read = ::recv(client_fd, data.buffer.data(),
                              data.buffer.size(), 0);
  if (bytes_read <= 0) {
    if (bytes_read == 0) {
      debug("Client disconnected during upload");
    } 
    perror("recv failed during upload");
    reset();
    close(client_fd);
    SocketUtils::remove_from_poll(client_fd);
    client_fd = -1; // Mark as closed
    return false;
  }
  // Resize the buffer to the actual amount of data read
  data.buffer.resize(static_cast<size_t>(bytes_read));
  debug("Received %ld bytes from client", bytes_read);
  return true;
}

/**
 * @brief Write the upload data to the file
 */
bool HTTPConnxData::writeUploadToFile() {
  ssize_t bytes_written =
      write(file_fd, data.buffer.data(), data.buffer.size());
  if (bytes_written <= 0) {
    perror(bytes_written < 0 ? "Failed to write to file"
                             : "No data written to file");
    reset();
    close(client_fd);
    SocketUtils::remove_from_poll(client_fd);
    client_fd = -1; // Mark as closed
    return false;
  }
  data.bytes_sent += static_cast<size_t>(bytes_written);
  debug("Wrote %ld bytes to file", bytes_written);
  debug("total bytes sent %zu/%zu", data.bytes_sent,
        data.content_length);
  data.buffer.clear();
  return true;
}

/**
 * @brief Send a simple response to the client
 *
 * @return true if the response was sent successfully, false otherwise
 */
bool HTTPConnxData::finishedSendingSimpleResponse() {
  data.response.reserve(Constants::BUFFER_SIZE);
  ssize_t bytes_sent = ::send(client_fd, data.response.c_str(),
                              data.response.size(), 0);
  if (bytes_sent < 0) {
    perror("Failed to send simple response");
    close_conn_after_error();
    return false;
  } else if (bytes_sent == 0) {
    debug("No data sent to client %d", client_fd);
  } else {
    debug("Sent %ld bytes to client %d", bytes_sent, client_fd);
    // defensive programming - handle partial send
    // Remove sent bytes from buffer
    data.response.erase(data.response.begin(),
                             data.response.begin() + bytes_sent);
    debug("Sent %zd bytes (%zu remaining in buffer)", bytes_sent,
          data.buffer.size());
    if (!data.response.empty()) {
      debug("Still data in response buffer %zu", data.response.size());
      return false;
    } else {
      debug("Finished sending response to client %d", client_fd);
      state = CONN_INCOMING;
      reset();
    }
  }
  return true;
}

/**
 * @brief Util function to close the connection after an error
 * mostly used in case of failed send or read
 */
void HTTPConnxData::close_conn_after_error() {
  SocketUtils::remove_from_poll(client_fd);
  reset();
  close(client_fd);
  SocketUtils::remove_from_poll(client_fd);
  client_fd = -1; // Mark as closed
}

/**
 * @brief Response headers are set if not already in place
 * 
 * Also checks for the presence of the HTTP/1.1 header
 */
bool HTTPConnxData::settingHeadersIfNeeded() {
  if (!headers_set) {
    if (!data.response.empty()) {
      assert(data.response.rfind("HTTP/1.1 ", 0) == 0 &&
             "Headers must start with 'HTTP/1.1 ");
      data.buffer.assign(data.response.begin(),
                              data.response.end());
      data.response.clear();
      headers_set = true;
      debug("Added headers for connection %d", client_fd);
    } else {
      close_conn_after_error();
      return false;
    }
  }
  return true;
}

/**
 * @brief Read new data from the file if the buffer is empty
 *
 */
bool HTTPConnxData::readNewDataFromFile() {
  // 1. Read new data if buffer is empty (and file not fully read)
  if (data.buffer.empty() && file_fd != -1) {
    char read_buf[Constants::BUFFER_SIZE];
    ssize_t bytes_read = read(file_fd, read_buf, sizeof(read_buf));

    if (bytes_read < 0) {
      perror("Failed to read file");
      close_conn_after_error();
      return false;
    } else if (bytes_read == 0) {
      debug("End of file reached for connection %d", client_fd);
      close(file_fd);
      file_fd = -1;
      // keep going, there might be more data in the buffer to send to
      // client
    } else {
      // Append new data to buffer
      data.buffer.insert(data.buffer.end(), read_buf,
                              read_buf + bytes_read);
    }
  }
  return true;
}

bool HTTPConnxData::sendNewDataFromFileToClient() {
  // 2. Send data from buffer (if any)
  if (!data.buffer.empty()) {
    ssize_t bytes_sent = ::send(client_fd, data.buffer.data(),
                                data.buffer.size(), 0);

    if (bytes_sent < 0) {
      perror("Failed to send data");
      debuglog(RED, "Error during file transfer for connection %d",
               client_fd);
      close_conn_after_error();
      return false;
    } else if (bytes_sent == 0) {
      debug("No data sent to client %d", client_fd);
    }
    // Remove sent bytes from buffer
    if (bytes_sent > 0) {
      data.bytes_sent += static_cast<size_t>(bytes_sent);
      data.buffer.erase(data.buffer.begin(),
                             data.buffer.begin() + bytes_sent);
      debug("Sent %zd bytes (%zu remaining in buffer)", bytes_sent,
            data.buffer.size());
    }
  }
  return true;
}

/**
 * @brief Check completion conditions for file transfer
 *
 * @param conn The connection data
 *
 * This function checks if the file transfer is complete. If the file
 * descriptor is -1 and the buffer is empty, it means the file has been
 * sent completely. In this case, it resets the connection and
 * updates the state to INCOMING.
 */
void HTTPConnxData::checkCompletionConditions() {
  if (file_fd == -1 && data.buffer.empty()) {
    debug("File sent completely for connection %d", client_fd);
    debug("File transfer complete for connection %d sent %lu bytes",
          client_fd, data.bytes_sent);
    debuglog(YELLOW,
             "Back to state INCOMING - File transfer complete for "
             "connection %d",
             client_fd);
    reset();
  }
}


/**
 * @brief Check for client timeout
 *
 * @param conn The connection data
 * @param current_fd The current file descriptor
 * 
 * Since it is  the client that is hanging i thnk it is 
 * not necessary to send a error message. I just close the connection
 */
void HTTPConnxData::check_for_client_timeout() {
  // check for timeouts
  if (data.client_timeout == 0) {
    // first time exiting ther loop without finding the fd
    data.client_timeout = std::time(NULL);
  } else {
    // check if the timeout is reached
    if (std::time(NULL) - data.client_timeout >
        Constants::client_timeout) {
      debug("Client timeout reached");
      // When detecting a client timeout
      debuglog(YELLOW, "Closing the connection (fd %d)", client_fd);
      // here I am in a state where typically the client remains
      // in POLLOUT and state incoming... I just close the connection
      close_conn_after_error();
    }
  }
}

/**
 * @brief Check for child process timeout
 *
 * The child timeout is the cgi process timeout. 
 */
bool HTTPConnxData::check_for_child_timeout() {
  debug("checking for child timeout");
  
  if (cgiData.child_timeout == 0) {
    cgiData.child_timeout = std::time(NULL);
  } else {
    // check if the timeout is reached
    debug("child timeout %ld", cgiData.child_timeout);
    if (std::time(NULL) - cgiData.child_timeout >
        Constants::cgi_child_timeout) {
      debug("CGI timeout reached");
      debuglog(YELLOW, "Child timeout reached for connection %d", client_fd);
      errorStatus = 504;
      closeConnection = true;
      state = CONN_CGI_FINISHED;
    }
  }
  return true;
}

/**
 * @brief Sends data (read from CGI stdout) to the client socket.
 *
 * @param bytes_read The number of bytes previously read from CGI stdout.
 * @return false if state changes to CONN_CGI_FINISHED or a critical error occurs,
 *         true if the send was successful (fully or partially) and state remains CONN_CGI_SENDING.
 */
bool HTTPConnxData::sendCgiDataToClient(ssize_t bytes_read) {
  // Ensure there's actually data to send (bytes_read should be > 0)
  if (bytes_read <= 0) {
      debuglog(RED, "sendCgiDataToClient called with bytes_read <= 0");
      state = CONN_CGI_FINISHED; 
      return false;
  }
  ssize_t bytes_written =
      ::send(client_fd, cgiData.buffer.c_str(),
             static_cast<size_t>(bytes_read), MSG_NOSIGNAL);
  if (bytes_written < 0) {
    // Handle send error (e.g., client disconnected)
    perror("Failed to send data to client");
    debuglog(RED, "Failed to send data to client fd %d", client_fd);
    state = CONN_CGI_FINISHED;
    return false; 
  } else  if (bytes_written == 0) {
    // Should generally not happen with blocking sockets unless client closed connection cleanly?
    debuglog(YELLOW, "Wrote 0 bytes to client fd %d (client likely closed connection)", client_fd);
    state = CONN_CGI_FINISHED;
    return false;
  }
  debug("Sent %ld bytes to client fd %d", bytes_written, client_fd);  
  // TODO revisit this logic. without it does not work properly at school?
  if (static_cast<size_t>(bytes_written) < Constants::BUFFER_SIZE) {
    debuglog(YELLOW, "Finished sending data chunk to client fd %d", client_fd);
    state = CONN_CGI_FINISHED;
    return false; // to mean we are done sending
  }
  return true; // Indicate send occurred, state remains CONN_CGI_SENDING
  // TODO: Handle partial writes (bytes_written < bytes_read) if using non-blocking sockets
  // For now, assume full write or error based on original code structure.
  // if (static_cast<size_t>(bytes_written) < static_cast<size_t>(bytes_read)) {
  //     debuglog(YELLOW, "Partial send to client fd %d (%ld / %ld bytes). Needs handling!",
  //              bytes_written, bytes_read);
  //     // State remains CONN_CGI_SENDING, but buffer needs adjustment for next POLLOUT
  //     // This requires more complex buffer management not present in original code.
  //     // For now, returning true, but this is incomplete for robust partial sends.
  // }
}


/**
 * @brief Write data to the child process stdin
 */
void HTTPConnxData::write_to_child_stdin(int current_fd, int pollfd) {
  ssize_t bytes_written =
      ::write(cgiData.cgi_stdin_fd, cgiData.buffer.c_str(),
              cgiData.buffer.size());
  debug("Wrote %ld bytes to CGI stdin", bytes_written);

  if (bytes_written < 0) {
    perror("Failed to write to CGI stdin");
    debug("Failed to write to CGI stdin");
    state = CONN_CGI_FINISHED;
    errorStatus = 500;
  } else if (bytes_written == 0) {
    // Should not happen with blocking write unless size was 0
    debuglog(YELLOW, "Wrote 0 bytes to CGI stdin (buffer size: %zu)",
             cgiData.buffer.size());
    debuglog(RED, "Wrote 0 bytes to CGI stdin unexpectedly.");
    state = CONN_CGI_FINISHED;
    errorStatus = 500;
    cgiData.buffer.clear();
  } else if (bytes_written < cgiData.buffer.size()) {
    // Partial write: Remove written data and wait for next POLLOUT
    debug("Partial write: Wrote %ld bytes to CGI stdin (buffer size: %zu)",
          bytes_written, cgiData.buffer.size());
    cgiData.buffer.erase(
        0, static_cast<std::string::size_type>(bytes_written));
    cgiData.bytes_received += static_cast<size_t>(bytes_written);
    // stay in the same state, poll will trigger again
  } else if (bytes_written == cgiData.buffer.size()) {
    // Full write (bytes_written == cgiData.buffer.size())
    debugcolor(MAGENTA, "wrote request buffer to CGI: %s",
               cgiData.buffer.c_str()); // Log data before clearing
    cgiData.bytes_received += static_cast<size_t>(bytes_written);
    cgiData.buffer.clear();
    debug("Full write: Wrote %ld bytes to CGI stdin", bytes_written);
  }
  if (cgiData.bytes_received >= data.content_length) {
    debug("Full write: Wrote %ld bytes to CGI stdin", bytes_written);
    // If we have written all data, clear the buffer
    cgiData.buffer.clear();
    cgiData.bytes_received = 0;
    // close the write end of the pipe to signal EOF to the CGI
    debuglog(YELLOW, "Closing write end of pipe");
    SocketUtils::remove_from_poll(cgiData.cgi_stdin_fd);
    close(cgiData.cgi_stdin_fd);
    cgiData.cgi_stdin_fd = -1; // Mark as closed
    state = CONN_CGI_SENDING;
  }
}

/**
 * @brief Get the directory listing for a given path
 * 
 * It will check with the server config if the directory listing is enabled
 * and if the path is valid. It will return false invalid
 */
bool HTTPConnxData::getDIRListing(string full_path) {
  if (data.target.empty() || data.target[0] != '/') {
    debuglog(RED, "Invalid target path: %s", data.target.c_str());
    return false;
  }
  // Check if directory exists
  DIR *dir = opendir(full_path.c_str());
  if (dir == NULL)
    return false;
  std::string dirString;
  // Read directory contents
  struct dirent *entry;
  // Ensure target path ends with a slash for proper URL construction
  std::string target_path = data.target;
  if (target_path.length() > 1 &&
      target_path[target_path.length() - 1] != '/') {
    target_path += '/';
  }
  debuglog(YELLOW, "Directory Listing using URL base: %s", target_path.c_str());
  while ((entry = readdir(dir)) != NULL) {
    dirString += "<li><a href=\"";
    dirString += target_path;
    // Skip adding target path if we're at root and it's already "/"
    if (target_path != "/" || entry->d_name[0] != '\0') {
      dirString += entry->d_name;
    }
    dirString += "\">";
    dirString += entry->d_name;
    dirString += "</a></li>\n";
  }
  closedir(dir);

  std::string htmlCode;
  htmlCode = "<html><head><title>Directory Listing</title>";
  htmlCode += "<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">";
  htmlCode += "</head><body><div style=\"text-align: left;\"><h1 style=\"margin: 0px;\">Index of " + data.target +
              "</h1><ul>" + dirString + "</ul></div></body></html>";

  debuglog(GREEN, "Directory contents: \n%s", dirString.c_str());
  urlMatcherData.content_type = Constants::mimeTypes[".html"];
  state = CONN_SIMPLE_RESPONSE;

  // generate HTTP header and include html payload using the stored content type
  Responses::createResponse(*this, urlMatcherData.content_type,
                            htmlCode, 200);
  return true;
}