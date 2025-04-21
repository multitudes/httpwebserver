#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

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
      : upload_dir(""),     // 5
        autoindex(false),   // 4 same priority because different
        file_upload(false), // 4 same priority because different
        internal(false), 
        root(""),           // 5 check for new root yes no
        acceptedMethods(),  // 2 if post then could be upload - if not could be autoindex
        return_directive(), // 1st - return immediately
        error_pages() {}
};

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

struct ServerData : public BaseConf {
  std::string serverListenAddress;
  std::vector<uint16_t> ports;
  std::vector<std::string> server_names;
  std::string index;
  std::string root;
  bool parsedroot;
  std::map<std::string, Location> location_blocks;
  CGIData cgiData;
  std::vector<std::string> acceptedMethods;
  bool cgi_exists;
  bool has_locations;

  ServerData()
      : serverListenAddress("localhost"), index("index.html"), root("www"),
        parsedroot(false), cgi_exists(false), has_locations(false) {};

  bool hasCGI() const { return cgi_exists; }
  bool hasDirectives() const { return has_locations; }
};
