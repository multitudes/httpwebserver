#include "URLMatcher.hpp"
#include "CGI.hpp"
#include "Config.hpp" // For Config::getConfigByPort()
#include "Constants.hpp"
#include "DirectoryListing.hpp"
#include "HTTPConnxData.hpp"
#include "HTTPServer.hpp"
#include "Responses.hpp"
#include "debug.h"
#include "Utils.hpp"
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>

using std::string;

namespace URLMatcher
{
  /**
   * @brief Receives and processes initial request data
   * @param conn The connection data structure
   * @return true if processing should continue, false if request handling is
   * complete
   */
  bool receiveAndParseRequest(HTTPConnxData &conn)
  {
	debug("Entered function");
    char buffer[BUFFER_SIZE + 1];

    // Ensure BUFFER_SIZE > 0 for recv
    ssize_t bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);

    if (bytes_read <= 0)
    {
      if (bytes_read == 0)
      {
        debuglog(YELLOW, "URLMatcher: Client fd %d disconnected.",
                 conn.client_fd);
      }
      else
      {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			debug("No data available yet - keep in reading state");
			return false;
		}

		//
		debug("%s", strerror(errno));
        // perror("URLMatcher: recv failed");
      }
      SocketUtils::remove_from_poll(conn.client_fd);
      close(conn.client_fd);
      conn.reset();
      HTTPServer::connections.erase(conn.client_fd);
      return false;
    }

    // Null-terminate buffer safely
    buffer[bytes_read] = '\0';
    conn.data.request.append(buffer,
                             static_cast<std::string::size_type>(bytes_read));
    debuglog(YELLOW, "URLMatcher: Received %lu bytes for fd %d", bytes_read,
             conn.client_fd);

    switch (conn.parseHeaders(conn))
    {
    case PARSE_SUCCESS:
      debuglog(YELLOW, "Headers parsed successfully");
      conn.data.headers_received = true;
      break;
    case PARSE_INCOMPLETE:
      debuglog(YELLOW, "Headers incomplete");
      conn.state = CONN_PARSING_HEADER;
      return false;
    case PARSE_ERROR:
      debuglog(RED, "Error parsing headers");
      SocketUtils::remove_from_poll(conn.client_fd);
      conn.reset();
      conn.state = CONN_SIMPLE_RESPONSE;
      debuglog(RED, "Error parsing headers");
      return false;
    }

    //   debuglog(MAGENTA, "Parsed connection data: %s",
    //    conn.formatConnectionDataLong(conn.data).c_str());
    debugcolor(MAGENTA, "Parsed whole connection data: %s",
               conn.data.request.c_str());
    return true;
  }

  /**
   * @brief Gets configuration and constructs the target path
   * @param conn The connection data structure
   * @return true if processing should continue, false if request handling is
   * complete
   */
  bool getConfigSetURLMatcherData(HTTPConnxData &conn)
  {
    conn.urlMatcherData.config = Config::getConfigByPort(conn.data.port);
    if (!conn.urlMatcherData.config)
    {
      debuglog(RED, "URLMatcher: No config found for port %d!", conn.data.port);
      Responses::htmlErrorResponse(conn, 500); // Internal Server Error
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return false;
    }

    string target = conn.data.target;
    if (!target.empty() && target[0] == '/')
    {
      target = target.substr(1);
    }

    // Basic directory traversal check
    if (target.find("..") != string::npos)
    {
      debuglog(RED, "URLMatcher: Directory traversal attempt detected: %s",
               conn.data.target.c_str());
      Responses::htmlErrorResponse(conn, 400); // Bad Request
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return false;
    }

    conn.urlMatcherData.full_path = conn.urlMatcherData.config->root + target;
    conn.urlMatcherData.path_for_stat = conn.urlMatcherData.full_path;
    conn.urlMatcherData.autoindex = conn.urlMatcherData.config->autoindex;
    conn.urlMatcherData.acceptedMethods = conn.urlMatcherData.config->acceptedMethods;
    conn.urlMatcherData.file_upload_dir = conn.urlMatcherData.config->upload_dir;

    // Adjust path_for_stat: remove trailing slash unless it's just the root path
    if (conn.urlMatcherData.path_for_stat.length() > conn.urlMatcherData.config->root.length() + 1 &&
        conn.urlMatcherData.path_for_stat[conn.urlMatcherData.path_for_stat.length() - 1] == '/')
    {
      conn.urlMatcherData.path_for_stat.erase(conn.urlMatcherData.path_for_stat.length() - 1, 1);
    }

    debuglog(YELLOW, "URLMatcher: Constructed path for stat: '%s'",
             conn.urlMatcherData.path_for_stat.c_str());
    debuglog(YELLOW, "URLMatcher: Original full path for dir checks: '%s'",
             conn.urlMatcherData.full_path.c_str());

    return true;
  }

  /**
   * @brief Determines content type based on file extension and stores it in the
   * connection
   * @param conn The connection data structure
   * @param path The file path to analyze
   */
  void determineContentType(HTTPConnxData &conn, const string &path)
  {
    // Default to generic binary type
    conn.urlMatcherData.content_type = "application/octet-stream";

    string file_extension = "";
    size_t dot_position = path.rfind('.');

    if (dot_position != string::npos)
    {
      file_extension = path.substr(dot_position);
      // Convert to lowercase for case-insensitive comparison
      for (size_t i = 0; i < file_extension.length(); i++)
      {
        file_extension[i] = static_cast<char>(std::tolower(file_extension[i]));
      }

      debuglog(GREEN, "URLMatcher: Looking up MIME type for extension: '%s'",
               file_extension.c_str());

      // Check if we have a MIME type mapping for this extension
      if (Constants::mimeTypes.find(file_extension) !=
          Constants::mimeTypes.end())
      {
        conn.urlMatcherData.content_type = Constants::mimeTypes[file_extension];
        debuglog(GREEN, "URLMatcher: Found MIME type: %s",
                 conn.urlMatcherData.content_type.c_str());
      }
      else
      {
        debuglog(
            YELLOW,
            "URLMatcher: No MIME type found for extension: %s, using default",
            file_extension.c_str());
      }
    }
  }

  /**
   * @brief Handles serving a regular file
   * @param conn The connection data structure
   * @param path_for_stat The path to the file
   * @param path_stat The stat structure with file info
   * @return true if file was opened and prepared for sending
   */
  bool handleRegularFile(HTTPConnxData &conn, const string &path_for_stat,
                         const struct stat &path_stat)
  {
    debuglog(GREEN, "URLMatcher: Target is a regular file. Serving '%s'",
             path_for_stat.c_str());

    // Set the content type in the connection
    determineContentType(conn, path_for_stat);

    debuglog(YELLOW, "URLMatcher: File '%s' using MIME type '%s'",
             path_for_stat.c_str(), conn.urlMatcherData.content_type.c_str());

    conn.file_fd = open(path_for_stat.c_str(), O_RDONLY);
    if (conn.file_fd < 0)
    {
      perror("URLMatcher: Failed to open file");
      Responses::htmlErrorResponse(conn, 403); // Forbidden is a common reason
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return false;
    }

    conn.file_size = path_stat.st_size;
    conn.state = CONN_FILE_REQUEST;

    // Use the overloaded version that doesn't need the content type parameter
    Responses::prepareFileResponse(conn, conn.file_size);

    debuglog(GREEN,
             "URLMatcher: Set state to CONN_FILE_REQUEST for fd %d, size %ld",
             conn.client_fd, conn.file_size);
    SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
    return true;
  }

  /**
   * @brief Handles serving an index file from a directory
   * @param conn The connection data structure
   * @param index_file_path The path to the index file
   * @param index_stat The stat structure with file info
   * @return true if index file was opened and prepared for sending
   */
  bool handleIndexFile(HTTPConnxData &conn, const string &index_file_path,
                       const struct stat &index_stat)
  {
    debuglog(GREEN, "URLMatcher: Index file found. Serving '%s'",
             index_file_path.c_str());

    conn.file_fd = open(index_file_path.c_str(), O_RDONLY);
    if (conn.file_fd < 0)
    {
      perror("URLMatcher: Failed to open existing index file");
      Responses::htmlErrorResponse(conn, 500); // Internal Server Error
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return false;
    }

    // Set the content type in the connection
    determineContentType(conn, index_file_path);

    conn.file_size = index_stat.st_size;
    conn.state = CONN_FILE_REQUEST;

    // Use the overloaded version that doesn't need the content type parameter
    Responses::prepareFileResponse(conn, conn.file_size);

    debuglog(
        GREEN,
        "URLMatcher: Set state to CONN_FILE_REQUEST for index fd %d, size %ld",
        conn.client_fd, conn.file_size);
    SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
    return true;
  }

  /**
   * @brief Handles directory listing when autoindex is enabled
   * @param conn The connection data structure
   * @return true if directory was successfully processed
   */
  bool handleDirectoryListing(HTTPConnxData &conn)
  {
    if (!conn.urlMatcherData.autoindex)
    {
      debuglog(RED, "URLMatcher: Autoindex is disabled.");
      Responses::htmlErrorResponse(conn, 403); // Forbidden to list directory
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return false;
    }

    debuglog(YELLOW,
             "URLMatcher: Autoindex is enabled. Calling getDIRListing for '%s'.",
             conn.urlMatcherData.full_path.c_str());

    if (DirectoryListing::getDIRListing(conn, conn.urlMatcherData.full_path))
    {
      debuglog(GREEN,
               "URLMatcher: getDIRListing prepared listing response for fd %d.",
               conn.client_fd);
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return true;
    }
    else
    {
      debuglog(RED,
               "URLMatcher: getDIRListing returned false for fd %d (likely "
               "opendir error).",
               conn.client_fd);
      Responses::htmlErrorResponse(conn, 500); // Internal Server Error
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return false;
    }
  }

  bool findCGIPathAlias(HTTPConnxData &conn)
  {
    string cgi_path_alias = conn.urlMatcherData.config->cgiData.cgi_path_alias.first;
    string cgi_path = conn.urlMatcherData.config->cgiData.cgi_path_alias.second;

    // First check if a CGI alias is defined
    if (cgi_path_alias.empty())
    {
      debuglog(BLUE, "URLMatcher: No CGI alias defined in config.");
      return false;
    }

    if (conn.data.target.find(cgi_path_alias) == 0)
    { // found CGI alias
      debuglog(BLUE, "URLMatcher: CGI path alias: '%s' -> '%s'",
               cgi_path_alias.c_str(), cgi_path.c_str());
      debuglog(BLUE, "URLMatcher: CGI alias found. Target: %s",
               conn.data.target.c_str());
      // TODO, use cgi path
      conn.urlMatcherData.full_path = cgi_path + conn.data.target.substr(cgi_path_alias.length());
      conn.urlMatcherData.path_for_stat = conn.urlMatcherData.full_path;
      debuglog(BLUE, "URLMatcher: Updated full path to CGI: '%s'",
               conn.urlMatcherData.full_path.c_str());

      conn.state = CONN_CGI;
      debug("CGI request detected");
      // Start CGI process for this connection
      if (CGI::prepareCGI(conn) < 0)
      {
        conn.reset();
        Responses::createResponse(conn, "text/plain", "TODO: Should Call CGI from: " + conn.urlMatcherData.full_path, 200);
        SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
        return false;
      }
      return true;
    }
    else
    {
      debuglog(BLUE, "URLMatcher: No CGI alias found.");
      return false;
    }
  }

  void updateWithLocationBlockConfig(HTTPConnxData &conn)
  {
    // Check if the server config has any location blocks defined
    if (conn.urlMatcherData.config->has_locations)
    {
      debuglog(YELLOW, "URLMatcher: Checking %lu location blocks for URI '%s'", 
               conn.urlMatcherData.config->location_blocks.size(), conn.data.target.c_str());
      
      // Loop through all location blocks to find a matching one
      for (std::map<std::string, Location>::const_iterator location_pair =
               conn.urlMatcherData.config->location_blocks.begin();
           location_pair != conn.urlMatcherData.config->location_blocks.end(); ++location_pair)
      {
        debuglog(YELLOW, "URLMatcher: Checking location '%s' against target '%s'",
                location_pair->first.c_str(), conn.data.target.c_str());
                
        // Check if the target URL starts with this location path
        if (conn.data.target.find(location_pair->first) == 0)
        { 
          // Found a matching location block
          Location location = location_pair->second;
          debuglog(GREEN, "URLMatcher: Found matching location block for '%s'",
                   location_pair->first.c_str());
          debuglog(GREEN, "URLMatcher: Location root is '%s'",
                   location.root.c_str());

          // Only update paths if location's root is different from the server's root
          if (location.root != conn.urlMatcherData.config->root)
          {
            debuglog(RED, "URLMatcher: Overriding path with location block root");
            // Override paths only if root is different from server root
            conn.urlMatcherData.full_path = location.root + conn.data.target.substr(location_pair->first.length());
            conn.urlMatcherData.path_for_stat = conn.urlMatcherData.full_path;
            debuglog(RED, "URLMatcher: Updated full path to '%s'",
                     conn.urlMatcherData.full_path.c_str());
          }
          else
            debuglog(RED, "URLMatcher: Location root is same as server root, using default paths");

          // Set autoindex flag from location block
          conn.urlMatcherData.autoindex = location.autoindex;
          debuglog(YELLOW, "URLMatcher: Location block autoindex is %s",
                   conn.urlMatcherData.autoindex ? "enabled" : "disabled");

          // Set accepted methods from location block
          conn.urlMatcherData.acceptedMethods = location.acceptedMethods;
          debuglog(YELLOW, "URLMatcher: accepted methods updated to Location block accepted methods");

          // Check for return directive in location block
          if (location.return_directive.first != 0)
          {
            conn.urlMatcherData.return_directive = true;
            debuglog(YELLOW, "URLMatcher: Location block return directive found: %d %s", 
                     location.return_directive.first, location.return_directive.second.c_str());
            Responses::createResponse(conn, "text/plain", location.return_directive.second, 
                                          location.return_directive.first);
            SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
            return;
          }

          // Check for file upload settings in location block
          if (location.file_upload && !location.upload_dir.empty())
          {
            conn.urlMatcherData.file_upload = true;
            conn.urlMatcherData.file_upload_dir = location.upload_dir;
            debuglog(YELLOW, "URLMatcher: File upload enabled with directory: %s",
                     location.upload_dir.c_str());
          }

          // We found a match, so stop looking through location blocks
          debuglog(GREEN, "URLMatcher: Applied configuration from location block '%s'",
                  location_pair->first.c_str());
          return;
        }
      }
      
      // If we reach here, no matching location block was found
      debuglog(YELLOW, "URLMatcher: No matching location block found for '%s'", 
               conn.data.target.c_str());
    }
    else
    {
      debuglog(YELLOW, "URLMatcher: Server has no location blocks defined");
    }
  }

  /**
   * @brief Validates incoming request, handles file/directory serving.
   *        Prioritizes index file check, then autoindex check, then listing.
   * @param conn The connection data structure.
   */
  void validateRequest(HTTPConnxData &conn)
  {
	debug("entering function");
    if (!receiveAndParseRequest(conn))
      return; // Request handling complete or failed

    // Get server configuration and construct the standard target path
    if (!getConfigSetURLMatcherData(conn))
      return; // Request handling complete or failed

    // check if target contains CGI alias
    if (findCGIPathAlias(conn))
      return;

    updateWithLocationBlockConfig(conn);
    if (conn.urlMatcherData.return_directive)
      return;

    // Debug log to verify location blocks are being checked
    debuglog(MAGENTA, "URLMatcher: After location check - path_for_stat: '%s', autoindex: %s", 
             conn.urlMatcherData.path_for_stat.c_str(),
             conn.urlMatcherData.autoindex ? "true" : "false");

    // check if the request method is accepted (GET POST etc)
    if (std::find(conn.urlMatcherData.acceptedMethods.begin(),
                  conn.urlMatcherData.acceptedMethods.end(),
                  std::string(conn.data.method)) == conn.urlMatcherData.acceptedMethods.end())
    {
      debuglog(RED, "URLMatcher: Method '%s' not allowed in location '%s'",
               conn.data.method.c_str(), conn.urlMatcherData.full_path.c_str());
      Responses::htmlErrorResponse(conn, 405); // Method Not Allowed
      SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      return;
    }
    debug("accepted method found: %s", conn.data.method.c_str());

    if (conn.data.method == "POST" && conn.data.content_length > 0)
    { 
		debug("POST request detected");
      debuglog(YELLOW, "URLMatcher: Upload request detected.");
      // check if upload allowed
      if (!conn.urlMatcherData.file_upload)
      {
		debug("file upload not allowed");
        debuglog(RED, "URLMatcher: File upload not allowed in location '%s'",
                 conn.urlMatcherData.full_path.c_str());
        Responses::htmlErrorResponse(conn, 403); // Forbidden
        SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
        return;
      }
      debugcolor(MAGENTA, "opening file for upload: %s",
                 conn.urlMatcherData.full_path.c_str());
      conn.file_fd = open(conn.urlMatcherData.full_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (conn.file_fd < 0)
      {
        perror("URLMatcher: Failed to open file for upload");
        Responses::htmlErrorResponse(conn, 500); // Internal Server Error
        SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
        return;
      }

      std::string payload = conn.data.request.substr(conn.data.headers_end);
      if (!payload.empty())
      {
		debug("payload found %s", payload.c_str());
        conn.data.response = payload;
        conn.data.bytes_sent = 0;
      }
      conn.state = CONN_UPLOAD;
	  debug("setting state to CONN_UPLOAD");
      return;
    } // end POST

    if (conn.data.method == "GET")
    {
      struct stat path_stat;
      if (stat(conn.urlMatcherData.path_for_stat.c_str(), &path_stat) != 0)
      {
        perror("URLMatcher: stat failed");
        Responses::htmlErrorResponse(conn, 404); // Not Found
        SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
        return;
      }

      // Handle regular file
      if (S_ISREG(path_stat.st_mode))
      {
        handleRegularFile(conn, conn.urlMatcherData.path_for_stat, path_stat);
      }
      // Handle directory
      else if (S_ISDIR(path_stat.st_mode))
      {
        debuglog(YELLOW, "URLMatcher: Target is a directory '%s'",
                 conn.urlMatcherData.full_path.c_str());

        // First check for an index file
        string index_file_path = conn.urlMatcherData.full_path;
        if (index_file_path.empty() ||
            index_file_path[index_file_path.length() - 1] != '/')
        {
          index_file_path += '/';
        }
        index_file_path += conn.urlMatcherData.config->index;

        struct stat index_stat;
        debuglog(YELLOW, "URLMatcher: Checking for index file at '%s'",
                 index_file_path.c_str());

        // If index file exists, serve it
        if (stat(index_file_path.c_str(), &index_stat) == 0 &&
            S_ISREG(index_stat.st_mode))
        {
          handleIndexFile(conn, index_file_path, index_stat);
        }
        // Otherwise, try directory listing
        else
        {
          debuglog(YELLOW,
                   "URLMatcher: Index file '%s' not found or not regular. "
                   "Checking autoindex.",
                   conn.urlMatcherData.config->index.c_str());
          handleDirectoryListing(conn);
        }
      }
      // Handle other file types
      else
      {
        debuglog(RED, "URLMatcher: Path '%s' is not a regular file or directory.",
                 conn.urlMatcherData.path_for_stat.c_str());
        Responses::htmlErrorResponse(conn, 415); // Unsupported Media Type
        SocketUtils::update_poll_events(conn.client_fd, POLLOUT);
      }
    } // end GET
  } // end of validateRequest
 
} // end of namespace URLMatcher