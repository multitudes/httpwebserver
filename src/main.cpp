#include "Config.hpp"
#include "Constants.hpp"
#include "HTTPServer.hpp"
#include "debug.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  try {
    std::string configFile =
        Constants::default_config_file; // Default config path
    if (argc > 2) {
      std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
      return 1;
    } else if (argc == 2) {
      configFile = argv[1];
    }
    Config::initialize(configFile); // Explicit initialization
    HTTPServer::run(configFile);
    Config::cleanup();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
