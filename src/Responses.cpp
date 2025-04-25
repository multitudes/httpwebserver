#include "Responses.hpp"
#include "Config.hpp"
#include "Constants.hpp"
#include "HTTPConnxData.hpp"
#include "SocketUtils.hpp"
#include "URLMatcher.hpp"
#include "Utils.hpp"
#include "debug.h"
#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <string>

using std::string;

namespace Responses {

// Helper function to add session cookie headers (doesn't modify
// response_headers)
string addSessionCookieHeaders(HTTPConnxData &conn) {
  if (conn.data.has_session && !conn.data.session_id.empty()) {
    // Update last accessed time
    conn.data.session_last_accessed = time(NULL);

    // Only return cookie headers if not already in response_headers
    if (conn.data.response_headers.find("Set-Cookie: sessionid=") ==
        string::npos) {
      debuglog(YELLOW, "Response: Adding session cookie to headers");
      return "Set-Cookie: sessionid=" + conn.data.session_id +
             "; Path=/; HttpOnly\r\n";
    }
  }
  return "";
}

// Create a more comprehensive function that handles all headers
void addStandardHeaders(HTTPConnxData &conn, string &header, int statusCode,
                        const string &contentType, long contentLength) {
  // Status line
  header = "HTTP/1.1 " + Utils::to_string(statusCode) + " " +
           Constants::statusMessages[statusCode] + "\r\n";

  // Content type
  header += "Content-Type: " + contentType + "\r\n";

  // Session cookie (only if needed)
  if (conn.data.has_session) {
    header += "Set-Cookie: sessionid=" + conn.data.session_id +
              "; Path=/; HttpOnly\r\n";
  }

  // Add any additional headers
  if (!conn.data.response_headers.empty()) {
    header += conn.data.response_headers;
  }

  // Content length
  header +=
      "Content-Length: " + Utils::to_string(static_cast<int>(contentLength)) +
      "\r\n";
  header += "\r\n";
}

/**
 * @brief generate a simple HTML error response
 */
void htmlErrorResponse(HTTPConnxData &conn, int statusCode) {
  // Check if a custom error page is defined for this status code
  if (conn.config->error_pages.find(statusCode) !=
      conn.config->error_pages.end()) {
    std::map<int, std::string>::const_iterator it =
        conn.config->error_pages.find(statusCode);
    if (it != conn.config->error_pages.end()) {
      string errorPagePath = it->second;
      debuglog(GREEN, "Custom error page found: %s", errorPagePath.c_str());

      // Use the URLMatcher helper function to serve the custom error page
      if (serveCustomErrorPage(conn, errorPagePath, statusCode)) {
        // Error page was successfully prepared
        // The state is already set to CONN_FILE_REQUEST by serveCustomErrorPage
        debuglog(GREEN, "Serving custom error page with status %d", statusCode);
        return;
      }
      // If serving the custom error page failed, we'll fall through to the
      // default error response
      debuglog(
          RED,
          "Failed to serve custom error page, falling back to generated HTML");
    }
  }
  // If no custom error page is found or couldn't be read, generate a simple
  // HTML response
  generatedHTMLResponse(conn, statusCode);
}

/**
 * @brief generate a simple HTML response
 * @param conn The connection data structure
 * @param statusCode The HTTP status code
 */
void generatedHTMLResponse(HTTPConnxData &conn, int statusCode) {
  string htmlCode;
  string statusText = Constants::statusMessages[statusCode];

  // Set content type directly in the conn
  conn.urlMatcherData.content_type = Constants::mimeTypes[".html"];

  htmlCode = "<!DOCTYPE html>\n";
  htmlCode += "<html lang = \"en\">\n";
  htmlCode += "<head>\n";
  htmlCode += "<meta charset=\"UTF-8\">\n";
  htmlCode += "<meta name=\"viewport\" content=\"width=device-width, "
              "initial-scale=1.0\">\n";
  htmlCode += "<title>" + Utils::to_string(statusCode) + " " + statusText +
              "</title>\n";
  htmlCode += "<link rel=\"icon\" href=\"../../favicon/favicon.ico\" "
              "type=\"image/x-icon\">\n";
  htmlCode += "<style> body {";
  htmlCode += "display: flex; flex-direction: column; justify-content: center;";
  htmlCode += "align-items: center; height: 100vh; margin: 0; "
              "background-color: black; color: white";
  htmlCode += "} </style>";
  htmlCode += "</head>\n";
  htmlCode += "<body>\n";

  htmlCode += "<h1>" + Utils::to_string(statusCode) + "</h1>\n";
  htmlCode += "<p>" + statusText + "</p>\n";

  htmlCode += "</body>\n";
  htmlCode += "</html>\n";

  createResponse(conn, conn.urlMatcherData.content_type, htmlCode, statusCode);
}

/**
 * @brief addd HTTP header to the response and set the response
 */
void createResponse(HTTPConnxData &conn, std::string contentType,
                    std::string response, int statusCode) {
  debuglog(YELLOW, "Creating response with status %d", statusCode);

  // Handle redirect status codes
  if ((statusCode == 301 || statusCode == 302 || statusCode == 303 ||
       statusCode == 307 || statusCode == 308) &&
      !response.empty()) {
    // Store the Location header in response_headers so it's included by
    // addStandardHeaders
    conn.data.response_headers += "Location: " + response + "\r\n";

    // For redirects, we typically want an empty or minimal body
    response = "<html><body>Redirecting to " + response + "</body></html>";

    debuglog(YELLOW, "Redirect: Adding Location header for %s",
             response.c_str());
    // Set content type to HTML for the redirect message
    contentType = "text/html";
  }

  string header;
  addStandardHeaders(conn, header, statusCode, contentType,
                     static_cast<int>(response.size()));

  conn.data.response = header + response;
  conn.state = CONN_SIMPLE_RESPONSE;

  debuglog(GREEN, "Response headers:\n%s", header.c_str());
}

/**
 * @brief generate a simple text response
 */
void simpleStatusResponse(HTTPConnxData &conn, int statusCode) {
  string response;
  string statusText = Constants::statusMessages[statusCode];

  // Set content type directly in the conn
  conn.urlMatcherData.content_type = Constants::mimeTypes[".txt"];

  response = Utils::to_string(statusCode) + statusText;
  createResponse(conn, conn.urlMatcherData.content_type, response, statusCode);
}

void prepareFileResponse(HTTPConnxData &conn, long fileSize) {
  // Use the content type already stored in the conn
  string header;
  addStandardHeaders(conn, header, 200, conn.urlMatcherData.content_type,
                     fileSize);

  conn.data.response = header;
  conn.headers_set = false;
  conn.data.bytes_sent = 0;
  debug("File response headers prepared using stored content type: %s",
        conn.data.response.c_str());
  debuglog(
      GREEN, "File response headers prepared using stored content type: %s\n%s",
      conn.urlMatcherData.content_type.c_str(), conn.data.response.c_str());
}

bool serveCustomErrorPage(HTTPConnxData &conn, const string &errorPagePath,
                          int statusCode) {
  struct stat path_stat;
  if (stat(errorPagePath.c_str(), &path_stat) != 0 ||
      !S_ISREG(path_stat.st_mode)) {
    debuglog(
        RED,
        "URLMatcher: Custom error page not found or not a regular file: %s",
        errorPagePath.c_str());
    return false;
  }

  // Determine content type based on file extension
  URLMatcher::determineContentType(conn, errorPagePath);

  // Open the file
  int fd = open(errorPagePath.c_str(), O_RDONLY);
  if (fd < 0) {
    debuglog(RED, "URLMatcher: Failed to open custom error page: %s",
             errorPagePath.c_str());
    return false;
  }

  // Set up conn for serving the file
  conn.file_fd = fd;
  conn.urlMatcherData.file_size = path_stat.st_size;
  conn.state = CONN_FILE_REQUEST;

  // Prepare headers with the error status code
  string header;
  addStandardHeaders(conn, header, statusCode, conn.urlMatcherData.content_type,
                     conn.urlMatcherData.file_size);

  conn.data.response = header;
  conn.headers_set = false;
  conn.data.bytes_sent = 0;

  debuglog(GREEN, "URLMatcher: Custom error page prepared with status %d",
           statusCode);
  return true;
}

} // namespace Responses