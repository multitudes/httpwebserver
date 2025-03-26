#pragma once

#include <netdb.h>
#include <pthread.h>

#include "ConfigData.hpp"
#include <map>
#include <string>
#include <vector>

/**
 * @brief Server configuration provider
 *
 * This class will be responsible for reading the configuration
 * file and creating an array of ConfigData structs
 * to be used by the server. The implementation is to make it a
 * singleton class. It will be initialized once at the beginning of the program
 * and never changed in our implementation. we could add a update method
 * to the class to reload the configuration file later if we want to implement
 * this
 */
class Config {
public:
  static std::vector<ConfigData>& getConfigData();
  
  
  private:
  // Private constructor to prevent instantiation
  Config(std::string filename);
  Config(const Config &);
  Config &operator=(const Config &);
  ~Config() {}
  
  static void cleanup();

  static Config *instance_;
  static pthread_mutex_t mutex_;
  static std::vector<ConfigData> configs_;
  static std::string _filename;
};
