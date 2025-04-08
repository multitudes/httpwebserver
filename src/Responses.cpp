#include "Responses.hpp"
#include "HTTPConnxData.hpp"
#include "Constants.hpp"
#include "Config.hpp"
#include "debug.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <sstream>
#include "Utils.hpp"
#include "SocketUtils.hpp"
#include "URLMatcher.hpp"
#include <poll.h> // Add this include for POLLOUT
using std::string;

namespace Responses {

    // addd HTTP header to the response and set the response
    void createResponse(HTTPConnxData &connection, std::string contentType, std::string response, int statusCode)
    {
        string header = "HTTP/1.1 ";
        header += Utils::to_string(statusCode) + " " + Constants::statusMessages[statusCode] + "\r\n";
        header += "Content-Type: " + contentType;
        header += "\r\n";
        
        // Add Location header for redirect status codes (301, 302, 303, 307, 308)
        if ((statusCode == 301 || statusCode == 302 || statusCode == 303 || 
             statusCode == 307 || statusCode == 308) && !response.empty()) {
            header += "Location: " + response + "\r\n";
        }
        
        header += "Content-Length: " + Utils::to_string(static_cast<int>(response.size())) + "\r\n";
        header += "\r\n";

        connection.data.response = header + response;
        
        // Always set state to CONN_SIMPLE_RESPONSE for simple string responses
        connection.state = CONN_SIMPLE_RESPONSE;
        
        debuglog(GREEN, "Basic response prepared, state set to CONN_SIMPLE_RESPONSE");
    }



    // generate a simple HTML error response
    void htmlErrorResponse(HTTPConnxData &connection, int statusCode)
    {
        // Check if a custom error page is defined for this status code
        if (connection.urlMatcherData.config->error_pages.find(statusCode) != connection.urlMatcherData.config->error_pages.end())
        {
            // Replace 'auto' with the explicit iterator type
            std::map<int, std::string>::const_iterator it = connection.urlMatcherData.config->error_pages.find(statusCode);
            if (it != connection.urlMatcherData.config->error_pages.end())
            {
                string errorPagePath = it->second;
                debuglog(GREEN, "Custom error page found: %s", errorPagePath.c_str());

                // Use the URLMatcher helper function to serve the custom error page
                if (serveCustomErrorPage(connection, errorPagePath, statusCode))
                {
                    // Error page was successfully prepared
                    // The state is already set to CONN_FILE_REQUEST by serveCustomErrorPage
                    debuglog(GREEN, "Serving custom error page with status %d", statusCode);
                    return;
                }
                // If serving the custom error page failed, we'll fall through to the default error response
                debuglog(RED, "Failed to serve custom error page, falling back to generated HTML");
            }
        }
        // If no custom error page is found or couldn't be read, generate a simple HTML response
        generatedHTMLResponse(connection, statusCode);
    }

    void generatedHTMLResponse(HTTPConnxData &connection, int statusCode)
    {
        string htmlCode;
        string statusText = Constants::statusMessages[statusCode];
        
        // Set content type directly in the connection
        connection.urlMatcherData.content_type = Constants::mimeTypes[".html"];

        htmlCode = "<!DOCTYPE html>\n";
        htmlCode += "<html lang = \"en\">\n";
        htmlCode += "<head>\n";
        htmlCode += "<meta charset=\"UTF-8\">\n";
        htmlCode += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        htmlCode += "<title>" + Utils::to_string(statusCode) + " " + statusText + "</title>\n";
        htmlCode += "<link rel=\"icon\" href=\"../../favicon/favicon.ico\" type=\"image/x-icon\">\n";
        htmlCode += "<style> body {";
        htmlCode += "display: flex; flex-direction: column; justify-content: center;";
        htmlCode += "align-items: center; height: 100vh; margin: 0; background-color: black; color: white";
        htmlCode += "} </style>";
        htmlCode += "</head>\n";
        htmlCode += "<body>\n";

        htmlCode += "<h1>" + Utils::to_string(statusCode) + "</h1>\n";
        htmlCode += "<p>" + statusText + "</p>\n";

        htmlCode += "</body>\n";
        htmlCode += "</html>\n";
       
        createResponse(connection, connection.urlMatcherData.content_type, htmlCode, statusCode); 
    }

    // generate a simple text response
    void simpleStatusResponse(HTTPConnxData &connection, int statusCode)
    {
        string response;
        string statusText = Constants::statusMessages[statusCode];
        
        // Set content type directly in the connection
        connection.urlMatcherData.content_type = Constants::mimeTypes[".txt"];

        response = Utils::to_string(statusCode) + statusText;
        createResponse(connection, connection.urlMatcherData.content_type, response, statusCode); 
    }

    void prepareFileResponse(HTTPConnxData &conn, long fileSize)
    {
        // Use the content type already stored in the connection
        string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + conn.urlMatcherData.content_type + "\r\n";
        header += "Content-Length: " + Utils::to_string(static_cast<long long>(fileSize)) + "\r\n";
        // header += "Connection: close\r\n";
        header += "\r\n";
        
        conn.data.response = header;
        conn.headers_sent = false;
        conn.data.bytes_sent = 0;
        
        debuglog(GREEN, "File response headers prepared using stored content type: %s\n%s", 
                 conn.urlMatcherData.content_type.c_str(), conn.data.response.c_str());
    }

    bool serveCustomErrorPage(HTTPConnxData &conn, const string &errorPagePath, int statusCode)
    {
      struct stat path_stat;
      if (stat(errorPagePath.c_str(), &path_stat) != 0 || !S_ISREG(path_stat.st_mode))
      {
        debuglog(RED, "URLMatcher: Custom error page not found or not a regular file: %s", errorPagePath.c_str());
        return false;
      }
  
      // Determine content type based on file extension
      URLMatcher::determineContentType(conn, errorPagePath);
  
      // Open the file
      int fd = open(errorPagePath.c_str(), O_RDONLY);
      if (fd < 0)
      {
        debuglog(RED, "URLMatcher: Failed to open custom error page: %s", errorPagePath.c_str());
        return false;
      }
  
      // Set up connection for serving the file
      conn.file_fd = fd;
      conn.file_size = path_stat.st_size;
      conn.state = CONN_FILE_REQUEST;
      
      // Prepare headers with the error status code
      string header = "HTTP/1.1 ";
      header += Utils::to_string(statusCode) + " " + Constants::statusMessages[statusCode] + "\r\n";
      header += "Content-Type: " + conn.urlMatcherData.content_type + "\r\n";
      header += "Content-Length: " + Utils::to_string(conn.file_size) + "\r\n";
      header += "\r\n";
      
      conn.data.response = header;
      conn.headers_sent = false;
      conn.data.bytes_sent = 0;
      
      debuglog(GREEN, "URLMatcher: Custom error page prepared with status %d", statusCode);
      return true;
    }

}