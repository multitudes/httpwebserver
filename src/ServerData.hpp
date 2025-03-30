#ifndef CONFIG_STRUCTS_HPP
#define CONFIG_STRUCTS_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <stdint.h> 

struct CGIData {
  std::pair<std::string, std::string> cgi_path_alias;
  std::string upload_dir;

  CGIData() : cgi_path_alias(), upload_dir() {}
};

struct Location {
  std::vector<std::string> acceptedMethods;
  bool autoindex;
  bool file_upload;
  std::string upload_dir;
  std::string alias;
  bool internal;
  std::pair<int, std::string> return_directive;
  std::map<int, std::string> error_pages;

  Location() : 
    autoindex(false), 
    file_upload(false),
    upload_dir("/www/uploads"),
    alias(""), 
    internal(false), 
    return_directive(), 
    error_pages() {}
};

struct BaseConf {
  size_t maxBodySize;
  std::size_t maxConnections;
  int requestTimeout;
  int responseTimeout;
  int keepalive_timeout;
  std::map<std::string, std::string> defaultheaders;
  bool autoindex;
  bool file_server;
  std::vector<std::string> acceptedMethods;
  std::map<int, std::string> error_pages;
  std::string upload_dir;

  BaseConf() :
    maxConnections(100),
    requestTimeout(60),
    responseTimeout(60),
    keepalive_timeout(30),
    maxBodySize(10000000),
    autoindex(false),
    file_server(true),
    upload_dir("./www/uploads") {
      defaultheaders["Content-Type"] = "text/html";
      defaultheaders["Server"] = "nginx/1.18.0";
      defaultheaders["Connection"] = "keep-alive";
    }
};

struct ServerData : public BaseConf {
  std::string serverListenAddress;
  std::vector<uint16_t> ports;
  std::vector<std::string> server_names;
  std::string index;
  std::string root;
  std::map<std::string, Location> location_blocks;
  CGIData cgiData;
  std::vector<std::string> limit_except;
  bool cgi_exists;
  bool has_locations;

  ServerData() :
    BaseConf(),
    serverListenAddress("localhost"),
    index("index.html"),
    root("www"),
    cgi_exists(false),
    has_locations(false) {}

  bool hasCGI() const { return cgi_exists; }
  bool hasDirectives() const { return has_locations; }
};

struct HttpConfig {
  std::vector<ServerData> servers;

  HttpConfig() {}
};

#endif