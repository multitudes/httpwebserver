#include <stdexcept>
#include "Config.hpp"
#include "ConfigData.hpp"
#include "debug.h"

/** Initialize the static variables */
std::vector<ConfigData> Config::configs_;
Config *Config::instance_ = NULL;
std::string Config::_filename = "config/default.conf"; // Default filename
std::map<uint16_t, ConfigData *> Config::port_map_;

/**
 * @brief Constructor for the Config class
 *
 * It will get a filename and read the configuration file
 * or default to a default configuration file if none is provided.
 *
 * For now it is hardcoded.alignas
 * The configuration according to the subject is an array of servers
 */
Config::Config(std::string filename) {
  (void)filename;

  // First server configuration
  ConfigData server1;
  server1.keepalive_timeout = 5;
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
  server1.ports.push_back(4244);
  server1.ports.push_back(4245);
  /* test the servernames with curl -H "Host: myWebserver"
   * http://localhost:4244/ or curl -H "Host: someWebserver"
   * http://localhost:4244/ or curl -H "Host: myWebserver"
   * http://localhost:4245/ or curl -H "Host: someWebserver"
   * http://localhost:4245/ or nc localhost 4244 and GET / HTTP/1.1 Host:
   * myWebserver
   */
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
  ConfigData server2;
  server2.ports.push_back(4246);
  server2.server_names.push_back("myWebserver");
  server2.server_names.push_back("someWebserver");
  server2.root = "http/www2/";

  // Third server configuration
  ConfigData server3;
  server3.ports.push_back(4247);
  server3.server_names.push_back("myWebserver");
  server3.server_names.push_back("someWebserver");
  server3.root = "http/www3/";

  // Add all servers to configs_
  configs_.push_back(server1);
  configs_.push_back(server2);
  configs_.push_back(server3);
  // map ports to server configurations
  for (size_t i = 0; i < configs_.size(); ++i) {
    for (size_t j = 0; j < configs_[i].ports.size(); ++j) {
      port_map_[configs_[i].ports[j]] = &configs_[i];
    }
  }
  debuglog(YELLOW, "Config initialized with %zu servers", configs_.size());
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

std::vector<ConfigData> &Config::getConfigData(char *config_file) {
  if (instance_ == NULL) {
    instance_ = new Config(Config::_filename);
  }
  return Config::configs_;
}

const ConfigData *Config::getConfigByPort(uint16_t port) {
  if (instance_ == NULL) {
    instance_ = new Config(Config::_filename);
    if (!validate()) {
      cleanup();
      debuglog(RED, "Configuration validation failed");
      throw std::runtime_error("Invalid configuration");
    }
  }
  std::map<uint16_t, ConfigData *>::const_iterator it = port_map_.find(port);
  return (it != port_map_.end()) ? it->second : NULL;
}

bool Config::validate() {
  for (size_t i = 0; i < configs_.size(); ++i) {
    if (configs_[i].ports.empty()) {
      debuglog(RED, "Configuration error: No ports specified");
      return false;
    }
    if (configs_[i].root.empty()) {
      debuglog(RED, "Configuration error: Empty root directory");
      return false;
    }
  }
  return true;
}