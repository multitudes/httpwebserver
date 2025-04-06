#include "Config.hpp"
#include "HTTPServer.hpp"
#include <iostream>
#include <string>
#include "Constants.hpp"
#include "debug.h"



int main(int argc, char *argv[]) {
    try {
        std::string configFile = Constants::default_config_file; // Default config path
        if (argc > 2) {
            std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
            return 1;
        } else if (argc == 2) {
            configFile = argv[1]; 
        }
		Config::initialize(configFile); // Explicit initialization
        HTTPServer::run(configFile, AutoRload);
        Config::cleanup();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
