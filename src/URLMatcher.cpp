#include "URLMatcher.hpp"
#include "CGI.hpp"              // Include if needed for other logic
#include "DirectoryListing.hpp" // Include your original DirectoryListing header
#include "HTTPConnxData.hpp"
#include "HTTPServer.hpp" // For poll updates, SERVER_PORT, connections map etc.
#include "debug.h"
#include <sys/stat.h>   // For stat()
#include <fcntl.h>      // For open()
#include <unistd.h>     // For close(), fstat()
#include <sys/socket.h> // For recv()
#include <string>       // Make sure std::string is included
#include "SimpleResponse.hpp"
#include "Config.hpp" // For Config::getConfigByPort()

namespace URLMatcher
{

  /**
   * @brief Validates incoming request, handles file/directory serving.
   *        Prioritizes index file check, then autoindex check, then listing.
   * @param conn The connection data structure.
   */
  void validateRequest(HTTPConnxData &conn)
  {
    // ================================================================
    // 1. Receive Request & Parse Headers (Keep your existing logic)
    //    This assumes conn.data.request is populated and headers are
    //    parsed, setting conn.data.headers_received = true, and
    //    populating conn.data.method, conn.data.target etc.
    // ================================================================
    // Example placeholder for receiving/parsing if not done earlier:
    if (conn.data.request.empty() || !conn.data.headers_received)
    {
      char buffer[BUFFER_SIZE + 1];

      // Ensure BUFFER_SIZE > 0 for recv
      ssize_t bytes_read = recv(conn.client_fd, buffer, BUFFER_SIZE, 0);

      if (bytes_read <= 0)
      {
        if (bytes_read == 0)
        {
          debuglog(YELLOW, "URLMatcher: Client fd %d disconnected.", conn.client_fd);
        }
        else
        {
          perror("URLMatcher: recv failed");
        }
        HTTPServer::remove_from_poll(conn.client_fd);
        close(conn.client_fd);
        conn.reset();
        HTTPServer::connections.erase(conn.client_fd); // Assuming map exists and is accessible
        return;
      }
      // Null-terminate buffer safely
      buffer[bytes_read] = '\0';
      conn.data.request.append(buffer, static_cast<std::string::size_type>(bytes_read));
      debuglog(YELLOW, "URLMatcher: Received %lu bytes for fd %d", bytes_read, conn.client_fd);

    //   conn.data.request.append(buffer, bytes_read);
      switch (conn.parseHeaders(conn))
      {
      case PARSE_SUCCESS:
        debuglog(YELLOW, "Headers parsed successfully");
        break;
      case PARSE_INCOMPLETE:
        debuglog(YELLOW, "Headers incomplete");
        conn.state = CONN_PARSING_HEADER;
        return;
      case PARSE_ERROR:
        debuglog(RED, "Error parsing headers");
        HTTPServer::remove_from_poll(conn.client_fd);
        conn.reset();
        conn.state = CONN_SIMPLE_RESPONSE;
        debuglog(RED, "Error parsing headers");
      }

	//   formatConnectionData
	    debuglog(MAGENTA, "Parsed connection data: %s", conn.formatConnectionDataLong(conn.data).c_str());
      // TODO prepare error response
      //   SimpleResponse::htmlErrorResponse(conn, 400);
      // --- End Placeholder ---

      // ================================================================
      // 2. Get Configuration & Construct Path
      // ================================================================

      const ConfigData *config = Config::getConfigByPort(conn.data.port);
      if (!config)
      {
        debuglog(RED, "URLMatcher: No config found for port %d!", conn.data.port);
        SimpleResponse::htmlErrorResponse(conn, 500); // Internal Server Error
        HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
        return;
      }

      // build the full path to the target
      std::string target = conn.data.target;
      if (!target.empty() && target[0] == '/')
      {
        target = target.substr(1);
      }
      // Basic directory traversal check, to block attack method like ../../../etc/passwd
      if (target.find("..") != std::string::npos)
      {
        debuglog(RED, "URLMatcher: Directory traversal attempt detected: %s", conn.data.target.c_str());
        SimpleResponse::htmlErrorResponse(conn, 400); // Bad Request
        HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
        return;
      }
      std::string full_path = config->root + "/" + target;
      // path_for_stat is adjusted for reliable stat() calls (removes trailing slash usually)
      std::string path_for_stat = full_path;

      // Adjust path_for_stat (C++98 compliant): remove trailing slash unless it's just the root path itself
      if (path_for_stat.length() > config->root.length() + 1 && path_for_stat[path_for_stat.length() - 1] == '/')
      {
        path_for_stat.erase(path_for_stat.length() - 1, 1);
      }

      debuglog(YELLOW, "URLMatcher: Constructed path for stat: '%s'", path_for_stat.c_str());
      debuglog(YELLOW, "URLMatcher: Original full path for dir checks: '%s'", full_path.c_str());

      // ================================================================
      // 3. Stat the path to check existence and type
      // ================================================================
      struct stat path_stat;
      if (stat(path_for_stat.c_str(), &path_stat) != 0)
      {
        // File/Dir does not exist or stat failed (e.g., permissions on parent dir)
        perror("URLMatcher: stat failed");
        debuglog(RED, "URLMatcher: Path not found or inaccessible '%s'", path_for_stat.c_str());
        SimpleResponse::htmlErrorResponse(conn, 404); // Not Found
        HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
        return;
      }

      // ================================================================
      // 4. Handle based on path type
      // ================================================================

      // --- 4a. Handle Regular File ---
      if (S_ISREG(path_stat.st_mode))
      {
        debuglog(GREEN, "URLMatcher: Target is a regular file. Serving '%s'", path_for_stat.c_str());
        conn.file_fd = open(path_for_stat.c_str(), O_RDONLY); // Use path_for_stat
        if (conn.file_fd < 0)
        {
          perror("URLMatcher: Failed to open file");
          // Check errno? EACCES -> 403, ENOENT (less likely here) -> 404
          SimpleResponse::htmlErrorResponse(conn, 403); // Forbidden is a common reason
          HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
        }
        else
        {
          conn.file_size = path_stat.st_size; // Get size from initial stat
          conn.state = CONN_FILE_REQUEST;     // Set state for file sending logic
          conn.headers_sent = false;          // Reset flags for sending this file
          conn.data.bytes_sent = 0;
          debuglog(GREEN, "URLMatcher: Set state to CONN_FILE_REQUEST for fd %d, size %ld", conn.client_fd, conn.file_size);
          HTTPServer::update_poll_events(conn.client_fd, POLLOUT); // Signal ready to send
        }
      }
      // --- 4b. Handle Directory ---
      else if (S_ISDIR(path_stat.st_mode))
      {
        debuglog(YELLOW, "URLMatcher: Target is a directory '%s'", full_path.c_str()); // Use original path for context

        // --- Check for index file FIRST ---
        std::string index_file_path = full_path;
        // Ensure directory path ends with '/' before appending index name for consistency
        if (index_file_path.empty() || index_file_path[index_file_path.length() - 1] != '/')
        {
          index_file_path += '/';
        }
        index_file_path += config->index; // index name from config (e.g., "index.html")

        struct stat index_stat;
        debuglog(YELLOW, "URLMatcher: Checking for index file at '%s'", index_file_path.c_str());

        // Attempt to stat the index file
        if (stat(index_file_path.c_str(), &index_stat) == 0 && S_ISREG(index_stat.st_mode))
        {
          // --- Index file FOUND and is a regular file ---
          debuglog(GREEN, "URLMatcher: Index file found. Serving '%s'", index_file_path.c_str());
          conn.file_fd = open(index_file_path.c_str(), O_RDONLY);
          if (conn.file_fd < 0)
          {
            // This is unexpected if stat succeeded, likely permissions issue on index itself
            perror("URLMatcher: Failed to open existing index file");
            SimpleResponse::htmlErrorResponse(conn, 500); // Internal Server Error
            HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
          }
          else
          {
            conn.file_size = index_stat.st_size; // Size from index_stat
            conn.state = CONN_FILE_REQUEST;      // Set state to serve the index file
            conn.headers_sent = false;
            conn.data.bytes_sent = 0;
            debuglog(GREEN, "URLMatcher: Set state to CONN_FILE_REQUEST for index fd %d, size %ld", conn.client_fd, conn.file_size);
            HTTPServer::update_poll_events(conn.client_fd, POLLOUT); // Ready to send index file
          }
        }
        else
        {
          // --- Index file NOT FOUND or not a regular file ---
          debuglog(YELLOW, "URLMatcher: Index file '%s' not found or not regular. Checking autoindex.", config->index.c_str());

          // *** Check AUTOINDEX configuration ***
          if (!config->autoindex)
          {
            // Autoindex is disabled for this server/location
            debuglog(RED, "URLMatcher: Autoindex is disabled.");
            SimpleResponse::htmlErrorResponse(conn, 403); // Forbidden to list directory
            HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
          }
          else
          {
            // *** Autoindex is ENABLED: Try to list the directory ***
            debuglog(YELLOW, "URLMatcher: Autoindex is enabled. Calling getDIRListing for '%s'.", full_path.c_str());

            // *** Call your ORIGINAL getDIRListing ***
            // Pass the original directory path (full_path)
            if (DirectoryListing::getDIRListing(conn, full_path))
            {
              // --- Listing Prepared by getDIRListing ---
              // getDIRListing already set state to CONN_SIMPLE_RESPONSE and prepared response data
              debuglog(GREEN, "URLMatcher: getDIRListing prepared listing response for fd %d.", conn.client_fd);
              HTTPServer::update_poll_events(conn.client_fd, POLLOUT); // Ready to send listing
            }
            else
            {
              // --- getDIRListing Returned False ---
              // This likely means opendir() failed inside getDIRListing (permissions?)
              // because the autoindex check passed here.
              debuglog(RED, "URLMatcher: getDIRListing returned false for fd %d (likely opendir error).", conn.client_fd);
              SimpleResponse::htmlErrorResponse(conn, 500); // Internal Server Error (more specific than 403 now)
              HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
            }
          }
        } // End of 'else' block for index not found
      } // End of 'else if (S_ISDIR(...))' block

      // --- 4c. Handle Other File Types (symlinks to non-files, sockets, etc.) ---
      else
      {
        debuglog(RED, "URLMatcher: Path '%s' is not a regular file or directory.", path_for_stat.c_str());
        SimpleResponse::htmlErrorResponse(conn, 403); // Forbidden - don't serve unusual file types
        HTTPServer::update_poll_events(conn.client_fd, POLLOUT);
      }

      // If we reach here without setting POLLOUT, it might indicate an unhandled case
      // or waiting for more request body data (if applicable for non-GET requests).

    } // end of validateRequest
  }
} // end of namespace URLMatcher