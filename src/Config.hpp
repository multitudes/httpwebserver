#pragma once

#include "Parser.hpp"
#include "ServerData.hpp"
#include <exception>
#include <map>
#include <netdb.h>
#include <pthread.h>
#include <stdint.h> // C-style header because stdint is C++11>
#include <string>
#include <vector>

/**
 * @brief Server configuration provider
 *
 * This class will be responsible for reading the configuration
 * file and creating an array of ServerData structs
 * to be used by the server. The implementation is to make it a
 * singleton class. It will be initialized once at the beginning of the program
 * and never changed in our implementation. we could add a update method
 * to the class to reload the configuration file later if we want to implement
 * this
 */
class Config {
public:
  static void initialize(std::string &config_file);
  static const std::vector<ServerData> &getServerData();
  static const std::vector<ServerData> &getServerData(char *config_file);
  static const ServerData *getConfigByPort(uint16_t port);
  static void cleanup();

  // static void debugprintConfigs();

private:
  // Private constructor to prevent instantiation
  Config(std::string filename);
  Config(const Config &);
  Config &operator=(const Config &);
  ~Config();

  // better and clearer to have a func to validate acc to a set of rules
  static bool validate();

  static std::map<uint16_t, ServerData *> port_map_;
  static std::vector<ServerData> servers;
  static Config *instance_;
  static std::string _filename;
};
