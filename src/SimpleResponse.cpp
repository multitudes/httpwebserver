#include "SimpleResponse.hpp"
#include "HTTPConnxData.hpp"
#include "Constants.hpp"
#include "Config.hpp"
#include "debug.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <sstream>
#include "Utils.hpp"

using std::string;

namespace SimpleResponse {

    // addd HTTP header to the response and set the response
    void addHTTPHeader(HTTPConnxData &connection, string contentType, std::string response, int statusCode)
    {
        string header = "HTTP/1.1 ";
        header += Utils::to_string(statusCode) + " " + Constants::statusMessages[statusCode] + "\r\n";
        header += "Content-Type: " + contentType;
        header += "\r\n";
        header += "Content-Length: " + Utils::to_string(static_cast<int>(response.size())) + "\r\n";
        header += "\r\n";

        connection.data.response = header + response;
        
        debuglog(GREEN, "Basic response: \n%s", connection.data.response.c_str());
    }

    // generate a simple HTML error response
    void htmlErrorResponse(HTTPConnxData &connection, int statusCode)
    {
        string htmlCode;
        string statusText = Constants::statusMessages[statusCode];
        
        // Set content type directly in the connection
        connection.content_type = Constants::mimeTypes[".html"];

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
       
        addHTTPHeader(connection, connection.content_type, htmlCode, statusCode); 
    }

    // generate a simple text response
    void simpleStatusResponse(HTTPConnxData &connection, int statusCode)
    {
        string response;
        string statusText = Constants::statusMessages[statusCode];
        
        // Set content type directly in the connection
        connection.content_type = Constants::mimeTypes[".txt"];

        response = Utils::to_string(statusCode) + statusText;
        addHTTPHeader(connection, connection.content_type, response, statusCode); 
    }

    // New function to prepare headers for file responses
    void prepareFileResponse(HTTPConnxData &conn, string contentType, long fileSize)
    {
        // Store the content type in the connection for future reference
        conn.content_type = contentType;
        
        string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + contentType + "\r\n";
        header += "Content-Length: " + Utils::to_string(fileSize) + "\r\n";
        header += "Connection: close\r\n";
        header += "\r\n";
        
        conn.data.response = header;
        conn.headers_sent = false;
        conn.data.bytes_sent = 0;
        
        debuglog(GREEN, "File response headers prepared: \n%s", conn.data.response.c_str());
    }

    // Overloaded version that uses the content_type already stored in the connection
    void prepareFileResponse(HTTPConnxData &conn, long fileSize)
    {
        // Use the content type already stored in the connection
        string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: " + conn.content_type + "\r\n";
        header += "Content-Length: " + Utils::to_string(fileSize) + "\r\n";
        header += "Connection: close\r\n";
        header += "\r\n";
        
        conn.data.response = header;
        conn.headers_sent = false;
        conn.data.bytes_sent = 0;
        
        debuglog(GREEN, "File response headers prepared using stored content type: %s\n%s", 
                 conn.content_type.c_str(), conn.data.response.c_str());
    }

}