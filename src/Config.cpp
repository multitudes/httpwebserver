#include "Config.hpp"
#include "Constants.hpp"
#include "HTTPServer.hpp"
#include "ServerData.hpp"
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
  if (!validate()) {
    cleanup();
    debuglog(RED, "Configuration validation failed");
    throw std::runtime_error("Invalid configuration");
  }

  debuglog(GREEN, "\n\nConfig initialized with %zu servers\n\n",
           servers.size());
}

// private constructor to prevent instantiation
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
  }
  std::map<uint16_t, ServerData *>::const_iterator it = port_map_.find(port);
  return (it != port_map_.end()) ? it->second : NULL;
}

bool Config::validate() {
  if (servers.size() == 0) {
    debuglog(RED, "Configuration error: Empty server array");
    return false;
  }
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i].ports.empty()) {
      debuglog(RED, "Configuration error: No ports specified");
      return false;
    } else if (servers[i].server_names.empty()) {
      debuglog(RED, "Configuration error: No server names specified");
      return false;
    } else if (servers[i].root.empty()) {
      debuglog(RED, "Configuration error: Empty root directory");
      return false;
    }
  }
  return true;
}
