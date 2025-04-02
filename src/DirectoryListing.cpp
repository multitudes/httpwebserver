#include "DirectoryListing.hpp"
#include "HTTPConnxData.hpp"
#include "SimpleResponse.hpp"
#include "Config.hpp"
#include "HTTPServer.hpp"
#include "debug.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <sstream>
#include "Constants.hpp"

namespace DirectoryListing
{

    bool getDIRListing(HTTPConnxData &connection, std::string full_path)
    {
        if (connection.data.target.empty() || connection.data.target[0] != '/' )
        {
            debuglog(RED, "Invalid target path: %s", connection.data.target.c_str());
            return false;
        }

        debuglog(RED, "Directory Listing target: %s", full_path.c_str());

        // Check if directory exists 
        DIR *dir = opendir(full_path.c_str());
        if (dir == NULL)
            return false;

        std::string dirString;
        // Read directory contents
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            dirString += "<li><a href=\"";
            dirString += connection.data.target;
            dirString += entry->d_name;
            dirString += "\">";
            dirString += entry->d_name;
            dirString += "</a></li>\n";
        }
        closedir(dir);
        std::string htmlCode;
        // add html code to the directory string
        htmlCode = "<html><head><title>Directory Listing</title>";
        htmlCode += "<style>";
        htmlCode += "body {background-color: black; color: white;} a {color: lightblue;}";
        htmlCode += "</style>";
        htmlCode += "</head><body><h1>Index of " + full_path + "</h1><ul>" + dirString + "</ul></body></html>";
       // htmlCode += "</head><body><h1>" + connection.data.target + "</h1><ul>" + dirString + "</ul></body></html>";
        debuglog(GREEN, "Directory contents: \n%s", dirString.c_str());
        // update connection state and data
        connection.state = CONN_SIMPLE_RESPONSE;
        // generate HTTP header and include html payload
        string contentType = Constants::mimeTypes["html"];
        SimpleResponse::addHTTPHeader(connection, contentType, htmlCode, 200);

        debuglog(BLUE, "Directory simple response: \n%s", connection.data.response.c_str());

        return true;
    }

} // namespace DirectoryListing
