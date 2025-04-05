#include "Config.hpp"
#include "HTTPServer.hpp"
#include "ServerData.hpp"
#include "Constants.hpp"
#include "debug.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

/** Initialize the static variables */
std::vector<ServerData> Config::servers;
Config *Config::instance_ = NULL;
std::string Config::_filename = "config/default.conf"; // Default filename
std::map<uint16_t, ServerData *> Config::port_map_;

/**
 * @brief Constructor for the Config class
 *
 * It will get a filename and read the configuration file
 * or default to a default configuration file if none is provided.
 */

Config::Config(std::string filename) {
	if (filename.empty()) {
		_filename = Constants::default_config_file;
	} else {
		_filename = filename;
	}

  Parser::parse(_filename, servers, port_map_);
  
  debuglog(GREEN, "\n\nConfig initialized with %zu servers\n\n",
           servers.size());
}

Config::Config(const Config &) {}
Config &Config::operator=(const Config &) { return *this; }
Config::~Config() {}

void Config::cleanup() {
  delete instance_;
  instance_ = NULL;
  debuglog(YELLOW, "Server config data destroyed");
}

void Config::initialize(std::string &config_file) {
  if (instance_ == NULL) {
    instance_ = new Config(config_file);
  }
}

const std::vector<ServerData> &Config::getServerData() {
	if (_filename.empty()) {
	  _filename = Constants::default_config_file;
	}
	if (instance_ == NULL) {
	  instance_ = new Config(Config::_filename);
	}
	return Config::servers;
  }

const std::vector<ServerData> &Config::getServerData(char *config_file) {
  if (config_file == NULL) {
	  _filename = Constants::default_config_file;
  } 
  if (instance_ == NULL) {
    instance_ = new Config(Config::_filename);
  }
  return Config::servers;
}

const ServerData *Config::getConfigByPort(uint16_t port) {
  if (instance_ == NULL) {
    instance_ = new Config(Config::_filename);
    if (!validate()) {
      cleanup();
      debuglog(RED, "Configuration validation failed");
      throw std::runtime_error("Invalid configuration");
    }
  }
  std::map<uint16_t, ServerData *>::const_iterator it = port_map_.find(port);
  return (it != port_map_.end()) ? it->second : NULL;
}

bool Config::validate() {
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i].ports.empty()) {
      debuglog(RED, "Configuration error: No ports specified");
      return false;
    }
    if (servers[i].root.empty()) {
      debuglog(RED, "Configuration error: Empty root directory");
      return false;
    }
  }
  return true;
}


/*
Config::Config(std::string filename) {
  (void)filename;

  // First server configuration
  ServerData server1;
  server1.keepalive_timeout = 5;
  server1.autoindex = true;
  Location location;
  location.return_directive = std::make_pair(301, "http://42berlin.de/");
  server1.location_blocks["/42"] = location;
  location = Location();
  location.autoindex = true;
  server1.location_blocks["/43"] = location;
  location = Location();
  location.return_directive = std::make_pair(301, "/here/index.html");
  server1.location_blocks["/go"] = location;
  location = Location();
  location.file_upload = true;
  server1.location_blocks["/uploads"] = location;
  server1.ports.push_back(SERVER_PORT);
 // server1.ports.push_back(4245);
  /* test the servernames with curl -H "Host: myWebserver"
   * http://localhost:4244/ or curl -H "Host: someWebserver"
   * http://localhost:4244/ or curl -H "Host: myWebserver"
   * http://localhost:4245/ or curl -H "Host: someWebserver"
   * http://localhost:4245/ or nc localhost 4244 and GET / HTTP/1.1 Host:
   * myWebserver
   */
/*
server1.server_names.push_back("myWebserver");
server1.server_names.push_back("someWebserver");
server1.root = "html/www1";
server1.index = "index.html";
server1.error_pages.insert(
    std::make_pair(400, server1.root + "/error_pages/400.html"));
server1.error_pages.insert(
    std::make_pair(403, server1.root + "/error_pages/403.html"));
server1.error_pages.insert(
    std::make_pair(404, server1.root + "/error_pages/404.html"));
server1.error_pages.insert(
    std::make_pair(405, server1.root + "/error_pages/405.html"));
server1.error_pages.insert(
    std::make_pair(418, server1.root + "/error_pages/418.html"));
server1.error_pages.insert(
    std::make_pair(500, server1.root + "/error_pages/500.html"));
server1.error_pages.insert(
    std::make_pair(502, server1.root + "/error_pages/502.html"));

server1.upload_dir = "http/www1/uploads";
server1.maxBodySize = 100000000;
server1.acceptedMethods.push_back("GET");
server1.acceptedMethods.push_back("POST");
server1.acceptedMethods.push_back("DELETE");
server1.acceptedMethods.push_back("PUT");
server1.cgiData.cgi_path_alias = std::make_pair("/cgi-bin", "/cgi-bin");
server1.cgiData.upload_dir = "www/uploads";
// server1.cgiData = data;
// Second server configuration
ServerData server2;
server2.ports.push_back(4246);
server2.server_names.push_back("myWebserver");
server2.server_names.push_back("someWebserver");
server2.root = "http/www2/";

// Third server configuration
ServerData server3;
server3.ports.push_back(4247);
server3.server_names.push_back("myWebserver");
server3.server_names.push_back("someWebserver");
server3.root = "http/www3/";

// Add all servers to servers
servers.push_back(server1);
servers.push_back(server2);
servers.push_back(server3);
// map ports to server configurations
for (size_t i = 0; i < servers.size(); ++i) {
  for (size_t j = 0; j < servers[i].ports.size(); ++j) {
    port_map_[servers[i].ports[j]] = &servers[i];
  }
}
debuglog(YELLOW, "Config initialized with %zu servers", servers.size());
}*/
/**
 * @brief Prints all server configurations in the servers vector
 *
 * This function prints detailed information about all server configurations
 * including ports, server names, locations, CGI settings, etc.
//
*/
