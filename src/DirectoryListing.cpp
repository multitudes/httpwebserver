#include "DirectoryListing.hpp"
#include "HTTPConnxData.hpp"
#include "Config.hpp"
#include "debug.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <sstream>

namespace DirectoryListing
{

    std::string intToString(int n)
    {
        std::ostringstream oss;
        oss << n;
        return oss.str();
    }

    bool getDIRListing(HTTPConnxData &connection)
    {
        //  ConfigData config = Config::getConfigByPort(connection.data.port);

        std::string fullPath = "www" + connection.data.target;

        if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
        {
            fullPath += '/';
        }

        // Get C-string pointer after all modifications
        const char *path = fullPath.c_str();

        debuglog(RED, "Directory Listing target: %s", path);

        // Check if directory exists and is readable
        DIR *dir = opendir(path);

        if (dir == NULL)
        {
            debuglog(RED, "Failed to open directory: %s", path);
            connection.file_fd = open("www/error_pages/404.html", O_RDONLY);
            return false;
        }

        std::string dirString;
        // Read directory contents
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Create an HTML list item for each entry
            dirString += "<li><a href=\"";
            dirString += connection.data.target;
            dirString += "/";
            dirString += entry->d_name;
            dirString += "\">";
            dirString += entry->d_name;
            dirString += "</a></li>\n";
        }
        closedir(dir);

        // add html code to the directory string
        dirString = "<html><head><title>Directory Listing</title></head><body><h3>Directory Listing for "
                     + fullPath + "</h3><ul>"
                     + dirString + "</ul></body></html>";
        debuglog(GREEN, "Directory contents: \n%s", dirString.c_str());
        // update connection state and data
        connection.state = CONN_SIMPLE_RESPONSE;
        // generate HTTP header and include html payload
        connection.data.response = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: " +
                                   intToString(dirString.size()) + "\r\n\r\n" +
                                   dirString;

        debuglog(BLUE, "Directory simple response: \n%s", connection.data.response.c_str());

        // // Write directory contents to a temporary file
        // connection.file_fd = open("directory.html", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        // if (connection.file_fd == -1) {
        //     debuglog(RED, "Failed to open directory.html for writing");
        //     return false;
        // }
        // if (write(connection.file_fd, dirString.c_str(), dirString.size()) == -1) {
        //     debuglog(RED, "Failed to write directory contents to file");
        //     close(connection.file_fd);
        //     return false;
        // }

        // debuglog(YELLOW, "Opening file directory.html for fd %d", connection.file_fd);

        // // Make sure directory.html exists in the expected location
        // connection.file_fd = open("directory.html", O_RDONLY);
        // if (connection.file_fd == -1) {
        //     debuglog(RED, "Failed to open directory.html template");
        //     connection.file_fd = open("www/error_pages/404.html", O_RDONLY);
        //     return false;
        // }

        return true;
    }

} // namespace DirectoryListing