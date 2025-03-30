#include "DirectoryListing.hpp"
#include "HTTPConnxData.hpp"
#include "SimpleResponse.hpp"
#include "Config.hpp"
#include "debug.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <sstream>

namespace DirectoryListing
{

    bool getDIRListing(HTTPConnxData &connection)
    {
		connection.state = CONN_FILE_REQUEST;
		return false;
        //  ConfigData config = Config::getConfigByPort(connection.data.port);
        // TODO: Check connection to if the directory listing is allowed

    //     std::string fullPath = "html/www1" + connection.data.target;

    //     if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
    //     {
    //         fullPath += '/';
    //     }

    //     // Get C-string pointer after all modifications
    //     const char *path = fullPath.c_str();

    //     debuglog(RED, "Directory Listing target: %s", path);

    //     // Check if directory exists and is readable
    //     DIR *dir = opendir(path);

    //     if (dir == NULL)
    //     {
    //         debuglog(RED, "Failed to open directory: %s", path);
    //         connection.state = CONN_SIMPLE_RESPONSE;
    //         SimpleResponse::htmlErrorResponse(connection, 404);            
    //         return false;
    //     }

        std::string dirString;
        // Read directory contents
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Create an HTML list item for each entry
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
        htmlCode += "</head><body><h1>Index " + fullPath + "</h1><ul>" + dirString + "</ul></body></html>";
       // htmlCode += "</head><body><h1>" + connection.data.target + "</h1><ul>" + dirString + "</ul></body></html>";
        debuglog(GREEN, "Directory contents: \n%s", dirString.c_str());
        // update connection state and data
        connection.state = CONN_SIMPLE_RESPONSE;
        // generate HTTP header and include html payload
        SimpleResponse::addHTTPHeader(connection, TEXT_HTML, htmlCode, 200);

    //     debuglog(BLUE, "Directory simple response: \n%s", connection.data.response.c_str());

    //     return true;
    }

} // namespace DirectoryListing