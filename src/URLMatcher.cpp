#include "URLMatcher.hpp"
#include "CGI.hpp"
#include "Config.hpp" // For Config::getConfigByPort()
#include "Constants.hpp"
#include "HTTPConnxData.hpp"
#include "HTTPServer.hpp"
#include "Responses.hpp"
#include "SocketUtils.hpp"
#include "Utils.hpp"
#include "debug.h"
#include <algorithm>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

using std::string;

namespace URLMatcher {

/**
 * @brief Validates incoming request, handles file/directory serving.
 *        Prioritizes index file check, then autoindex check, then listing.
 * @param conn The connection data structure.
 */
void validateRequest(HTTPConnxData &conn) {
  if (!receiveAndParseRequest(conn))
    return;
  conn.config = Config::getConfigByPort(conn.data.port);
  if (!handleChunkedData(conn))
    return;
  if (handleCookieUpdateRequest(conn))
    return;
  if (!getConfigSetURLMatcherData(conn))
    return;
  if (findCGIPathAlias(conn))
    return;

  updateWithLocationBlockConfig(conn);
  if (conn.urlMatcherData.return_directive)
    return;

  // Validate method is allowed
  if (std::find(conn.urlMatcherData.acceptedMethods.begin(),
                conn.urlMatcherData.acceptedMethods.end(),
                std::string(conn.data.method)) ==
      conn.urlMatcherData.acceptedMethods.end()) {
    debuglog(RED, "URLMatcher: Method '%s' not allowed",
             conn.data.method.c_str());
    Responses::htmlErrorResponse(conn, 405);
    return;
  }

  // Route to appropriate handler
  if (conn.data.method == "GET") {
    handleGETRequest(conn);
  } else if (conn.data.method == "POST") {
    handlePOSTRequest(conn);
  } else if (conn.data.method == "DELETE") {
    handleDELETERequest(conn);
  }
}

/**
 * @brief Handles GET request for file serving
 * @param conn The connection data structure
 * @return true if the file was opened successfully, false otherwise
 */
bool handleGETRequest(HTTPConnxData &conn) {
  struct stat path_stat;
  if (stat(conn.urlMatcherData.path_for_stat.c_str(), &path_stat) != 0) {
    Responses::htmlErrorResponse(conn, 404);
    return false;
  }

  if (S_ISREG(path_stat.st_mode)) {
    return handleRegularFile(conn, conn.urlMatcherData.path_for_stat,
                             path_stat);
  } else if (S_ISDIR(path_stat.st_mode)) {
    debuglog(YELLOW, "URLMatcher: Target is a directory '%s'",
             conn.urlMatcherData.full_path.c_str());

    // First check for an index file
    string index_file_path = conn.urlMatcherData.full_path;
    if (index_file_path.empty() ||
        index_file_path[index_file_path.length() - 1] != '/') {
      index_file_path += '/';
    }
    index_file_path += conn.config->index;

    struct stat index_stat;
    debuglog(YELLOW, "URLMatcher: Checking for index file at '%s'",
             index_file_path.c_str());

    // If index file exists, serve it
    if (stat(index_file_path.c_str(), &index_stat) == 0 &&
        S_ISREG(index_stat.st_mode)) {
      handleIndexFile(conn, index_file_path, index_stat);
    }
    // Otherwise, try directory listing
    else {
      debuglog(YELLOW,
               "URLMatcher: Index file '%s' not found or not regular. "
               "Checking autoindex.",
               conn.config->index.c_str());
      handleDirectoryListing(conn);
    }
  } else {
    Responses::htmlErrorResponse(conn, 415);
    return false;
  }
  return true;
}

/**
 * @brief Handles POST request for file upload
 * @param conn The connection data structure
 * @return true if the file was opened successfully, false otherwise
 */
bool handlePOSTRequest(HTTPConnxData &conn) {
  if (conn.data.content_length == 0)
    return false;

  if (conn.data.content_length > conn.config->maxBodySize) {
    Responses::htmlErrorResponse(conn, 413);
    return false;
  }

  // Check if upload allowed
  if (!conn.urlMatcherData.file_upload) {
    debug("file upload not allowed");
    debuglog(RED, "URLMatcher: File upload not allowed in location '%s'",
             conn.urlMatcherData.full_path.c_str());
    Responses::htmlErrorResponse(conn, 403); // Forbidden

    return false;
  }
  debuglog(MAGENTA, "opening file for upload: %s",
             conn.urlMatcherData.full_path.c_str());
  conn.file_fd = open(conn.urlMatcherData.full_path.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (conn.file_fd < 0) {
    perror("URLMatcher: Failed to open file for upload");
    Responses::htmlErrorResponse(conn, 500); // Internal Server Error

    return false;
  }

  std::string payload = conn.data.request.substr(conn.data.headers_end);
  if (!payload.empty()) {
    debug("payload found %s", payload.c_str());
    conn.data.response = payload;
    conn.data.bytes_sent = 0;
  }
  conn.state = CONN_UPLOAD;
  debug("setting state to CONN_UPLOAD");
  return true;
}

/**
 * @brief Handles DELETE request for file deletion
 * @param conn The connection data structure
 * @return true if the file was deleted successfully, false otherwise
 */
bool handleDELETERequest(HTTPConnxData &conn) {
  if (!conn.urlMatcherData.file_upload) {
    Responses::htmlErrorResponse(conn, 403);
    return false;
  }

  int result = unlink(conn.urlMatcherData.full_path.c_str());
  if (result == 0) {
    Responses::createResponse(conn, "text/plain", "File deleted", 200);
    return true;
  }

  if (errno == ENOENT) {
    Responses::htmlErrorResponse(conn, 404);
  } else {
    Responses::createResponse(
        conn, "text/plain",
        "Failed to delete file: " + std::string(strerror(errno)), 500);
  }
  return false;
}

// handles %20 -> space, %2F -> /, $3F -> ?, +  -> space etc...
/**
 * @brief Decodes a URL-encoded string
 * @param encoded The URL-encoded string
 * @return The decoded string
 * handles %20 -> space, %2F -> /, $3F -> ?, +  -> space etc...
 */
string urlDecode(const string &encoded) {
  string decoded;
  for (size_t i = 0; i < encoded.length(); ++i) {
    if (encoded[i] == '%' && i + 2 < encoded.length()) {
      // Get the two hex characters after %
      string hex = encoded.substr(i + 1, 2);
      int value;
      std::istringstream hex_chars(hex);

      // Convert hex to decimal
      if (hex_chars >> std::hex >> value) {
        // Common URL encodings:
        // %20 = space (32)
        // %2F = / (47)
        // %3F = ? (63)
        // %3D = = (61)
        decoded += static_cast<char>(value);
        i += 2; // Skip the two hex chars
      } else {
        decoded += encoded[i]; // Invalid hex, keep the %
      }
    } else if (encoded[i] == '+') {
      decoded += ' '; // + in query strings means space
    } else {
      decoded += encoded[i]; // Normal character
    }
  }
  return decoded;
}

/**
 * @brief Receives and processes initial request data
 * @param conn The connection data structure
 * @return true if processing should continue, false if request handling is
 * complete
 */
bool receiveAndParseRequest(HTTPConnxData &conn) {
  debug("checking the request");
  char buffer[Constants::BUFFER_SIZE + 1];

  ssize_t bytes_read =
      ::recv(conn.client_fd, buffer, Constants::BUFFER_SIZE, 0);

  if (bytes_read <= 0) {
    if (bytes_read == 0) {
      debuglog(YELLOW, "URLMatcher: Client fd %d disconnected.",
               conn.client_fd);
      perror("Client closed connection");
    } else {
      perror("recv failed");
    }
    conn.reset();
    SocketUtils::remove_from_poll(conn.client_fd);
    close(conn.client_fd);
    conn.client_fd = -1; // Mark as closed
    return false;
  }

  conn.data.request.append(buffer,
                           static_cast<std::string::size_type>(bytes_read));
  debuglog(YELLOW, "URLMatcher: Received %lu bytes for fd %d", bytes_read,
           conn.client_fd);

  // Remove old chunking code and just handle headers
  switch (conn.parseHeaders()) {
  case HEADERS_PARSE_SUCCESS:
    debuglog(YELLOW, "Headers parsed successfully");
    conn.data.headers_received = true;
    conn.urlMatcherData.target = urlDecode(conn.data.target);
    debuglog(YELLOW, "Decoded target path: '%s'", conn.data.target.c_str());
    break;
  case HEADERS_PARSE_INCOMPLETE:
    debuglog(YELLOW, "Headers incomplete");
    conn.state = CONN_PARSING_HEADER;
    return false;
  case HEADERS_PARSE_ERROR:
    debuglog(RED, "Error parsing headers");
    debug("Error parsing headers");
    conn.reset();
    conn.state = CONN_INCOMING;
    HTTPServer::send_critical_error(conn.client_fd, 400);
    close(conn.client_fd);
    SocketUtils::remove_from_poll(conn.client_fd);
    conn.client_fd = -1; // Mark as closed
    return false;
  }
  debuglog(MAGENTA, "Parsed whole connection data: %s",
             conn.data.request.c_str());
  return true;
}

/**
 * @brief Gets configuration and constructs the target path
 * @param conn The connection data structure
 * @return true if processing should continue, false if request handling is
 * complete
 */
bool getConfigSetURLMatcherData(HTTPConnxData &conn) {
  if (!conn.config) {
    debuglog(RED, "URLMatcher: No config found for port %d!", conn.data.port);
    Responses::htmlErrorResponse(conn, 500); // Internal Server Error
    return false;
  }

  string target = conn.urlMatcherData.target;
  if (!target.empty() && target[0] == '/') {
    target = target.substr(1);
  }

  // Basic directory traversal check
  if (target.find("..") != string::npos) {
    debuglog(RED, "URLMatcher: Directory traversal attempt detected: %s",
             conn.data.target.c_str());
    Responses::htmlErrorResponse(conn, 400); // Bad Request

    return false;
  }

  // Construct the full path for the file or directory stat check
  conn.urlMatcherData.full_path = conn.config->root + target;
  conn.urlMatcherData.path_for_stat = conn.config->root;
  conn.urlMatcherData.path_for_stat =
      Utils::ensureTrailinSlash(conn.urlMatcherData.path_for_stat) + target;
  conn.urlMatcherData.autoindex = conn.config->autoindex;
  conn.urlMatcherData.acceptedMethods = conn.config->acceptedMethods;

  debuglog(YELLOW, "URLMatcher: Constructed path for stat: '%s'",
           conn.urlMatcherData.path_for_stat.c_str());
  debuglog(YELLOW, "URLMatcher: Original full path for dir checks: '%s'",
           conn.urlMatcherData.full_path.c_str());

  return true;
}

/**
 * @brief Determines content type based on file extension and stores it in the
 * connection
 * @param conn The connection data structure
 * @param path The file path to analyze
 */
void determineContentType(HTTPConnxData &conn, const string &path) {
  // Default to generic binary type
  conn.urlMatcherData.content_type = "application/octet-stream";

  string file_extension = "";
  size_t dot_position = path.rfind('.');

  if (dot_position != string::npos) {
    file_extension = path.substr(dot_position);
    // Convert to lowercase for case-insensitive comparison
    for (size_t i = 0; i < file_extension.length(); i++) {
      file_extension[i] = static_cast<char>(std::tolower(file_extension[i]));
    }

    debuglog(GREEN, "URLMatcher: Looking up MIME type for extension: '%s'",
             file_extension.c_str());

    // Check if we have a MIME type mapping for this extension
    if (Constants::mimeTypes.find(file_extension) !=
        Constants::mimeTypes.end()) {
      conn.urlMatcherData.content_type = Constants::mimeTypes[file_extension];
      debuglog(GREEN, "URLMatcher: Found MIME type: %s",
               conn.urlMatcherData.content_type.c_str());
    } else {
      debuglog(
          YELLOW,
          "URLMatcher: No MIME type found for extension: %s, using default",
          file_extension.c_str());
    }
  }
}

/**
 * @brief Handles serving a regular file
 * @param conn The connection data structure
 * @param path_for_stat The path to the file
 * @param path_stat The stat structure with file info
 * @return true if file was opened and prepared for sending
 */
bool handleRegularFile(HTTPConnxData &conn, const string &path_for_stat,
                       const struct stat &path_stat) {
  debuglog(GREEN, "URLMatcher: Target is a regular file. Serving '%s'",
           path_for_stat.c_str());

  // Set the content type in the connection
  determineContentType(conn, path_for_stat);

  debuglog(YELLOW, "URLMatcher: File '%s' using MIME type '%s'",
           path_for_stat.c_str(), conn.urlMatcherData.content_type.c_str());

  conn.file_fd = open(path_for_stat.c_str(), O_RDONLY);
  if (conn.file_fd < 0) {
    perror("URLMatcher: Failed to open file");
    Responses::htmlErrorResponse(conn, 403); // Forbidden is a common reason

    return false;
  }

  conn.urlMatcherData.file_size = path_stat.st_size;
  conn.state = CONN_FILE_REQUEST;

  Responses::prepareFileResponse(conn, conn.urlMatcherData.file_size);

  debuglog(GREEN,
           "URLMatcher: Set state to CONN_FILE_REQUEST for fd %d, size %ld",
           conn.client_fd, conn.urlMatcherData.file_size);

  return true;
}

/**
 * @brief Handles serving an index file from a directory
 * @param conn The connection data structure
 * @param index_file_path The path to the index file
 * @param index_stat The stat structure with file info
 * @return true if index file was opened and prepared for sending
 */
bool handleIndexFile(HTTPConnxData &conn, const string &index_file_path,
                     const struct stat &index_stat) {
  debuglog(GREEN, "URLMatcher: Index file found. Serving '%s'",
           index_file_path.c_str());

  conn.file_fd = open(index_file_path.c_str(), O_RDONLY);
  if (conn.file_fd < 0) {
    perror("URLMatcher: Failed to open existing index file");
    Responses::htmlErrorResponse(conn, 500); // Internal Server Error

    return false;
  }

  // Set the content type in the connection
  determineContentType(conn, index_file_path);

  conn.urlMatcherData.file_size = index_stat.st_size;
  conn.state = CONN_FILE_REQUEST;

  // Use the overloaded version that doesn't need the content type parameter
  Responses::prepareFileResponse(conn, conn.urlMatcherData.file_size);

  debuglog(
      GREEN,
      "URLMatcher: Set state to CONN_FILE_REQUEST for index fd %d, size %ld",
      conn.client_fd, conn.urlMatcherData.file_size);

  return true;
}

/**
 * @brief Handles directory listing when autoindex is enabled
 * @param conn The connection data structure
 * @return true if directory was successfully processed
 */
bool handleDirectoryListing(HTTPConnxData &conn) {
  if (!conn.urlMatcherData.autoindex) {
    debuglog(RED, "URLMatcher: Autoindex is disabled.");
    Responses::htmlErrorResponse(conn, 404); // index not found

    return false;
  }

  debuglog(YELLOW,
           "URLMatcher: Autoindex is enabled. Calling getDIRListing for '%s'.",
           conn.urlMatcherData.full_path.c_str());

  if (conn.getDIRListing(conn.urlMatcherData.full_path)) {
    debuglog(GREEN,
             "URLMatcher: getDIRListing prepared listing response for fd %d.",
             conn.client_fd);

    return true;
  } else {
    debuglog(RED,
             "URLMatcher: getDIRListing returned false for fd %d (likely "
             "opendir error).",
             conn.client_fd);
    Responses::htmlErrorResponse(conn, 500); // Internal Server Error

    return false;
  }
}

/**
 * @brief Checks if the CGI extension is allowed
 * @param conn The connection data structure
 * @param path The path to check
 * @return true if the extension is allowed, false otherwise
 */
bool isAllowedCGIExtension(const HTTPConnxData &conn, const string &path) {
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == string::npos) {
        debuglog(RED, "URLMatcher: No file extension found for CGI script");
        return false;
    }

    // Find the next slash or question mark after the dot
    size_t end_pos = path.find_first_of("/?", dot_pos);
    // If no slash or question mark found, use the whole remaining string
    string extension;
    if (end_pos == string::npos) {
        extension = path.substr(dot_pos);
    } else {
        extension = path.substr(dot_pos, end_pos - dot_pos);
    }

    for (std::vector<std::string>::const_iterator it = conn.config->cgiData.cgi_extensions.begin();
         it != conn.config->cgiData.cgi_extensions.end(); ++it) {
        if (*it == extension) {
            return true;
        }
    }
    debuglog(RED, "URLMatcher: CGI extension '%s' not allowed", extension.c_str());
    return false;
}

/**
 * @brief Checks if the target path matches the CGI path alias
 * @param conn The connection data structure
 * @return true if CGI path alias is found, false otherwise
 */
bool findCGIPathAlias(HTTPConnxData &conn) {
  // Get CGI path mappings from config
  string cgi_path_alias = conn.config->cgiData.cgi_path_alias.first;
  string cgi_path = conn.config->cgiData.cgi_path_alias.second;

  if (cgi_path_alias.empty()) {
    return false;
  }

  if (conn.data.target == cgi_path_alias ||
      (conn.data.target.find(Utils::ensureTrailinSlash(cgi_path_alias)) == 0)) {

    // Map URL path to CGI path
    string relative_path = conn.data.target.substr(cgi_path_alias.length());
    conn.urlMatcherData.full_path = cgi_path + relative_path;
    conn.cgiData.script_name = conn.urlMatcherData.full_path;

    // Check extension is allowed
    if (!isAllowedCGIExtension(conn, relative_path)) {
        debuglog(RED, "URLMatcher: CGI extension forbidden");
        Responses::htmlErrorResponse(conn, 403);
        return true;
    }

    // Build filesystem path for stat check
    string check_path = conn.config->root;
    if (!check_path.empty() && check_path[check_path.length() - 1] == '/') {
        check_path = check_path.substr(0, check_path.length() - 1);
    }
    check_path += "/" + conn.urlMatcherData.full_path;

    // Check if file exists
    struct stat script_stat;
    if (stat(check_path.c_str(), &script_stat) != 0 || !S_ISREG(script_stat.st_mode)) {
        debuglog(RED, "URLMatcher: CGI script not found");
        Responses::htmlErrorResponse(conn, 404);
        return true;
    }

    // Check executable permissions
    if (!(script_stat.st_mode & S_IXUSR)) {
        debuglog(RED, "URLMatcher: CGI script not executable");
        Responses::htmlErrorResponse(conn, 403);
        return true;
    }

    // All checks passed, execute script
    conn.state = CONN_CGI_INCOMING;
    if (CGI::prepareCGI(conn) < 0) {
        conn.reset();
        Responses::createResponse(conn, "text/plain", "Failed to execute CGI script", 500);
        return true;
    }
    return true;
  }
  return false;
}

/**
 * @brief Updates the connection paths based on the location block
 * @param conn The connection data structure
 * @param location The location block to apply
 * @param locationPath The path of the location block
 */
void updatePathsFromLocation(HTTPConnxData &conn, const Location &location,
                             const std::string &locationPath) {
  if (location.root != conn.config->root) {
    debuglog(RED, "URLMatcher: Overriding path with location block root");
    conn.urlMatcherData.full_path =
        location.root + conn.data.target.substr(locationPath.length());
    conn.urlMatcherData.path_for_stat = conn.urlMatcherData.full_path;
    debuglog(RED, "URLMatcher: Updated full path to '%s'",
             conn.urlMatcherData.full_path.c_str());
  }
}

/**
 * @brief Applies location block settings to the connection
 * @param conn The connection data structure
 * @param location The location block to apply
 * @return true if a return directive was found, false otherwise
 */
bool applyLocationBlockSettings(HTTPConnxData &conn, const Location &location) {
  conn.urlMatcherData.autoindex = location.autoindex;
  conn.urlMatcherData.acceptedMethods = location.acceptedMethods;

  if (location.return_directive.first != 0) {
    conn.urlMatcherData.return_directive = true;
    Responses::createResponse(conn, "text/plain",
                              location.return_directive.second,
                              location.return_directive.first);
    return true;
  }

  if (location.file_upload) {
    conn.urlMatcherData.file_upload = true;
  }
  return false;
}

/**
 * @brief Updates the connection with location block settings
 * @param conn The connection data structure
 */
void updateWithLocationBlockConfig(HTTPConnxData &conn) {
  if (!conn.config->has_locations) {
    debuglog(YELLOW, "URLMatcher: Server has no location blocks defined");
    return;
  }

  for (std::map<std::string, Location>::const_iterator location_pair =
           conn.config->location_blocks.begin();
       location_pair != conn.config->location_blocks.end(); ++location_pair) {

    if (conn.data.target.find(location_pair->first) != 0)
      continue;

    Location location = location_pair->second;
    updatePathsFromLocation(conn, location, location_pair->first);

    if (applyLocationBlockSettings(conn, location))
      return;

    debuglog(GREEN,
             "URLMatcher: Applied configuration from location block '%s'",
             location_pair->first.c_str());
    return;
  }

  debuglog(YELLOW, "URLMatcher: No matching location block found for '%s'",
           conn.data.target.c_str());
}

/**
 * @brief Handles cookie update requests
 * @param conn The connection data structure
 * @return true if the request was handled, false otherwise
 */
bool handleCookieUpdateRequest(HTTPConnxData &conn) {
  if (conn.data.target.find("/api/update-cookie/") == 0) {
    debuglog(YELLOW, "Original target: '%s'", conn.data.target.c_str());

    // Skip past "/api/update-cookie/"
    size_t prefixLength = strlen("/api/update-cookie/");
    string fullPath = conn.data.target.substr(prefixLength);
    debuglog(YELLOW, "After prefix removal: '%s'", fullPath.c_str());

    // Find the first forward slash after prefix
    size_t separator = fullPath.find("/");
    if (separator == string::npos) {
      debuglog(RED, "Invalid cookie update request format");
      Responses::simpleStatusResponse(conn, 400);
      return true;
    }

    // Extract the name and value parts
    string cookieName = fullPath.substr(0, separator);   // Get "buttonClicked"
    string cookieValue = fullPath.substr(separator + 1); // Get "true"

    debuglog(YELLOW, "Cookie components:");
    debuglog(YELLOW, "  Name: '%s'", cookieName.c_str());
    debuglog(YELLOW, "  Value: '%s'", cookieValue.c_str());

    // Create session if needed
    if (!conn.data.has_session) {
      debuglog(MAGENTA, "Creating new session for cookie update request");
      conn.createSession();
    }

    // Format cookie header with correct name=value format
    string cookieHeader =
        "Set-Cookie: " + cookieName + "=" + cookieValue + "; Path=/\r\n";
    debuglog(YELLOW, "Generated cookie header: '%s'", cookieHeader.c_str());

    conn.data.response_headers = cookieHeader;

    // Create success response
    Responses::createResponse(conn, "application/json",
                              "{\"status\":\"success\"}", 200);
    debuglog(GREEN, "Cookie update request completed successfully");
    return true;
  }
  return false;
}

/**
 * @brief Handles chunked transfer encoding
 * @param conn The connection data structure
 * @return true if chunked data was processed successfully, false otherwise
 */
bool handleChunkedData(HTTPConnxData &conn) {
  if (!conn.data.headers_received || !conn.data.chunked) {
    return true; // Not chunked, continue processing
  }

  debuglog(YELLOW, "Processing chunked data...");

  // Check for end marker
  if (conn.data.request.find("0\r\n\r\n") == string::npos) {
    debuglog(YELLOW, "Still reading chunked data");
    conn.state = CONN_RECV_CHUNKS;
    return false;
  }

  // Get data after headers
  string chunked_string = conn.data.request.substr(conn.data.headers_end);
  string dechunked = conn.dechunkData(chunked_string);

  // Replace the request payload with dechunked data
  conn.data.request =
      conn.data.request.substr(0, conn.data.headers_end) + dechunked;

  // Update headers and state
  conn.data.chunked = false;
  conn.data.headers["Content-Length"] = Utils::to_string(dechunked.length());
  conn.data.content_length = dechunked.length();
  conn.data.headers.erase("Transfer-Encoding");

  debuglog(GREEN, "Successfully dechunked data (size: %zu)",
           dechunked.length());
  return true;
}

} // end of namespace URLMatcher