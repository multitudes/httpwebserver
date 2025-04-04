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
        
        // Ensure target path ends with a slash for proper URL construction
        std::string target_path = connection.data.target;
        if (target_path.length() > 1 && target_path[target_path.length() - 1] != '/') {
            target_path += '/';
        }
        
        debuglog(YELLOW, "Directory Listing using URL base: %s", target_path.c_str());
        
        while ((entry = readdir(dir)) != NULL)
        {
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
        // add html code to the directory string
        htmlCode = "<html><head><title>Directory Listing</title>";
        htmlCode += "<style>";
        htmlCode += "body {background-color: black; color: white;} a {color: lightblue;}";
        htmlCode += "</style>";
        htmlCode += "</head><body><h1>Index of " + connection.data.target + "</h1><ul>" + dirString + "</ul></body></html>";
        
        debuglog(GREEN, "Directory contents: \n%s", dirString.c_str());
        
        // Set content type directly in the connection
        connection.urlMatcherData.content_type = Constants::mimeTypes[".html"];
        
        // update connection state
        connection.state = CONN_SIMPLE_RESPONSE;
        
        // generate HTTP header and include html payload using the stored content type
        SimpleResponse::createResponse(connection, connection.urlMatcherData.content_type, htmlCode, 200);

        debuglog(BLUE, "Directory simple response: \n%s", connection.data.response.c_str());

        return true;
    }

} // namespace DirectoryListing
