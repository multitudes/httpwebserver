
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

namespace SimpleResponse {

    void addHTTPHeader(HTTPConnxData &connection, int content_type, std::string response)
    {
        std::string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: ";
        if (content_type == TEXT_PLAIN)
            header += "text/plain";
        else if (content_type == TEXT_HTML)
            header += "text/html";
        header += "\r\n";
        header += "Content-Length: " + Utils::to_string(response.size()) + "\r\n";
        header += "\r\n";
        connection.data.response = header + response;
        debuglog(GREEN, "Basic response: \n%s", connection.data.response.c_str());
    }

    // <!DOCTYPE html>
    // <html lang = "en">
    //     <head>
    //         <meta charset="UTF-8">
    //         <meta name="viewport" content="width=device-width, initial-scale=1.0">
    //         <title>404 Not Found</title>
    //         <link rel="icon" href="../favicon/favicon.ico" type="image/x-icon">
    //         <link rel="stylesheet" type="text/css" href="../css/style.css">
    //     </head>
    //     <body>
    //         <div class="rectangle">
    //             <div class="circle">
    //                 <h1>404</h1>
    //                 <p>Page Not Found</p>
    //             </div>
    //             <p>The address you were looking for cannot be found or is not valid</p>
    //         </div>
    //     </body>
    // </html>

    void htmlErrorResponse(HTTPConnxData &connection, int statusCode)
    {
        std::string htmlCode;
        std::string statusText = Constants::statusMessages[statusCode];

        htmlCode = "<!DOCTYPE html>\n";
        htmlCode += "<html lang = \"en\">\n";
        htmlCode += "<head>\n";
        htmlCode += "<meta charset=\"UTF-8\">\n";
        htmlCode += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        htmlCode += "<title>" + Utils::to_string(statusCode) + " " + statusText + "</title>\n";
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
        addHTTPHeader(connection, TEXT_HTML, htmlCode); 

    }
}