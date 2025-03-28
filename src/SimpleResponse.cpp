
#include "SimpleResponse.hpp"
#include "HTTPConnxData.hpp"
#include "Config.hpp"
#include "debug.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <sstream>

namespace SimpleResponse

{
    std::string intToString(int n)
    {
        std::ostringstream oss;
        oss << n;
        return oss.str();
    }

    void setErrorText(int errorCode, std::string &errorType, std::string &errorText)
    {

        switch (errorCode)
        {
        case 301:
            errorType = "Moved Permanently";
            errorText = "The requested resource has been assigned a new permanent URI and any future references to this resource should be done using one of the returned URIs.";
            break;
        case 404:
            errorType = "Page Not Found";
            errorText = "The address you were looking for cannot be found or is not valid";
            break;
        case 405:
            errorType = "Method Not Allowed";
            errorText = "The method specified in the Request-Line is not allowed for the resource identified by the Request-URI.";
            break;
        default:
            errorType = "Internal Server Error";
            errorText = "The server encountered an unexpected condition which prevented it from fulfilling the request.";
        }
    }

    void addHTTPHeader(HTTPConnxData &connection, int content_type, std::string response)
    {
        std::string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: ";
        if (content_type == TEXT_PLAIN)
            header += "text/plain";
        else if (content_type == TEXT_HTML)
            header += "text/html";
        header += "\r\n";
        header += "Content-Length: " + intToString(response.size()) + "\r\n";
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

    void htmlErrorResponse(HTTPConnxData &connection, int errorCode)
    {
        std::string htmlCode;
        std::string errorType;
        std::string errorText;
        setErrorText(errorCode, errorType, errorText);
        htmlCode = "<!DOCTYPE html>\n";
        htmlCode += "<html lang = \"en\">\n";
        htmlCode += "<head>\n";
        htmlCode += "<meta charset=\"UTF-8\">\n";
        htmlCode += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        htmlCode += "<title>" + intToString(errorCode) + " " + errorType + "</title>\n";
        // htmlCode += "<link rel=\"icon\" href=\"../favicon/favicon.ico\" type=\"image/x-icon\">\n";
        // htmlCode += "<link rel=\"stylesheet\" type=\"text/css\" href=\"../css/style.css\">\n";
        htmlCode += "</head>\n";
        htmlCode += "<body>\n";
        htmlCode += "<div class=\"rectangle\">\n";
        htmlCode += "<div class=\"circle\">\n";
        htmlCode += "<h1>" + intToString(errorCode) + "</h1>\n";
        htmlCode += "<p>" + errorType + "</p>\n";
        htmlCode += "</div>\n";
        htmlCode += "<p>" + errorText + "</p>\n";
        htmlCode += "</div>\n";
        htmlCode += "</body>\n";
        htmlCode += "</html>\n";
        addHTTPHeader(connection, TEXT_HTML, htmlCode); 

    }
}