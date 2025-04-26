#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

/* ---- All data structs here are used to contain the parsed config */

/**
 * @brief CGIData struct for the cgi location in the server block
 *
 * It defaults the accepted methods to GET, POST, DELETE, and PUT
 */
struct CGIData {
  std::pair<std::string, std::string> cgi_path_alias;
  std::string upload_dir;
  std::vector<std::string> cgi_extensions;
  std::vector<std::string> acceptedMethods;

  CGIData() : cgi_path_alias(), upload_dir() {
    acceptedMethods.push_back("GET");
    acceptedMethods.push_back("POST");
    acceptedMethods.push_back("DELETE");
    acceptedMethods.push_back("PUT");
  }
};

/**
 * @brief Location struct for the location blocks in the server block
 *
 * We can have a location block used as uplod directory, autoindex, with a
 * different root, accepted methods, return directive. CGI is handles
 * separately.
 */
struct Location {
  std::string upload_dir;
  bool autoindex;
  bool file_upload;
  bool internal;
  std::string root;
  std::vector<std::string> acceptedMethods;
  std::pair<int, std::string> return_directive;
  std::map<int, std::string> error_pages;

  Location()
      : upload_dir(""),            // 5
        autoindex(false),          // 4 same priority because different
        file_upload(false),        // 4 same priority because different
        internal(false), root(""), // 5 check for new root yes no
        acceptedMethods(),  // 2 if post then could be upload - if not could be
                            // autoindex
        return_directive(), // 1st - return immediately
        error_pages() {}
};

/**
 * @brief BaseConf struct for the global settings
 *
 * It contains the default headers, max body size, autoindex,
 * file server, accepted methods, error pages and upload directory
 * which are common for all server blocks.
 */
struct BaseConf {
  size_t maxBodySize;
  std::map<std::string, std::string> defaultheaders;
  bool autoindex;
  bool file_server;
  std::vector<std::string> acceptedMethods;
  std::map<int, std::string> error_pages;
  std::string upload_dir;

  BaseConf()
      : maxBodySize(10000000), autoindex(false), 
      file_server(true),
       upload_dir("./html/www1/upload") {
    defaultheaders["Content-Type"] = "text/html";
    defaultheaders["Server"] = "webserv/1.0";
    defaultheaders["Connection"] = "keep-alive";
    acceptedMethods.push_back("GET");
    acceptedMethods.push_back("POST");
    acceptedMethods.push_back("DELETE");
    acceptedMethods.push_back("PUT");
  }
};

/**
 * @brief ServerData struct for the server blocks
 *
 * It contains the server listen address, ports, server names,
 * index, root, location blocks, cgi data and accepted methods.
 */
struct ServerData : public BaseConf {
  CGIData cgiData;
  std::string serverListenAddress;
  std::vector<uint16_t> ports;
  std::vector<std::string> server_names;
  std::string index;
  std::string root;
  bool parsedroot;
  std::map<std::string, Location> location_blocks;
  std::vector<std::string> acceptedMethods;
  bool cgi_exists;
  bool has_locations;

  ServerData()
      : serverListenAddress("localhost"), index("index.html"), root("www"),
        parsedroot(false), cgi_exists(false), has_locations(false) {};

  bool hasCGI() const { return cgi_exists; }
  bool hasDirectives() const { return has_locations; }
};
