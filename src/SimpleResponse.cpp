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
        string contentType = Constants::mimeTypes["html"];

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
       
        addHTTPHeader(connection, contentType, htmlCode, statusCode); 
    }

    // generate a simple text response
    void simpleStatusResponse(HTTPConnxData &connection, int statusCode)
    {
        string response;
        string statusText = Constants::statusMessages[statusCode];
        string contentType = Constants::mimeTypes["text"];

        response = Utils::to_string(statusCode) + statusText;
        addHTTPHeader(connection, contentType, response, statusCode); 
    }


}