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
 *
 * For now it is hardcoded.alignas
 * The configuration according to the subject is an array of servers
 */

Config::Config(std::string filename) {
	if (filename.empty()) {
		_filename = Constants::default_config_file;
	} else {
		_filename = filename;
	}

  std::ifstream configFile(_filename.c_str());
  if (!configFile.is_open()) {
    throw std::runtime_error("Failed to open config file: " + _filename);
  }

  // Read entire file into a string
  std::stringstream buffer;
  buffer << configFile.rdbuf();
  std::string content = buffer.str();
  configFile.close();

  // Create an HttpConfig to hold all parsed data
  HttpConfig httpConfig;
  BaseConf baseConfig;

  // Find the HTTP block
  size_t httpStart = content.find("http {");
  if (httpStart == std::string::npos) {
    throw std::runtime_error("No http block found in configuration");
  }

  size_t httpEnd =
      findClosingBrace(content, httpStart + 6); // +6 to skip "http {"
  if (httpEnd == std::string::npos) {
    throw std::runtime_error("Unclosed http block in configuration");
  }

  // Extract HTTP block content
  std::string httpContent =
      content.substr(httpStart + 6, httpEnd - (httpStart + 6));

  // Parse global settings first
  parseGlobalSettings(httpContent, baseConfig);

  // Parse server blocks
  parseServerBlocks(httpContent, httpConfig, baseConfig);

  // Copy the parsed servers to the static servers vector
  servers = httpConfig.servers;

  // check duplicate port
  removeDuplicatePorts();

  for (size_t i = 0; i < servers.size(); ++i) {
    for (size_t j = 0; j < servers[i].ports.size(); ++j) {
      port_map_[servers[i].ports[j]] = &servers[i];
    }
  }

  configValidate();

  // debugprintConfigs();

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

template <typename T>
bool parseNumericValue(const std::string &line, const std::string &param,
                       size_t paramLen, T &outValue) {
  size_t valueStart = line.find_first_not_of(" \t", paramLen);
  size_t valueEnd = line.find(';', valueStart);

  if (valueEnd != std::string::npos) {
    std::string valueStr = line.substr(valueStart, valueEnd - valueStart);
    outValue = static_cast<T>(atoi(valueStr.c_str()));
    return true;
  }
  return false;
}

size_t findClosingBrace(const std::string &content, size_t start) {
  int braceCount = 1;
  for (size_t i = start; i < content.length(); ++i) {
    if (content[i] == '{') {
      ++braceCount;
    } else if (content[i] == '}') {
      --braceCount;
      if (braceCount == 0) {
        return i;
      }
    }
  }
  return std::string::npos;
}

void Config::parseGlobalSettings(const std::string &httpContent,
                                 BaseConf &baseConfig) {
  std::istringstream iss(httpContent);
  std::string line;

  while (std::getline(iss, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
      continue; // Skip empty lines
    size_t end = line.find_last_not_of(" \t\n\r");
    std::string trimmedLine = line.substr(start, end - start + 1);

    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue; // Skip comments
    }

    // Check for global settings
    if (trimmedLine.find("maxBodySize") == 0) {
      size_t maxBodySize;
      if (parseNumericValue(trimmedLine, "maxBodySize", 11, maxBodySize)) {
        if (maxBodySize <= 0) {
          debuglog(YELLOW,
                   "Warning: Invalid maxBodySize value: %lu, using default",
                   maxBodySize);
          // Keep default value
        } else {
          baseConfig.maxBodySize = maxBodySize;
          debuglog(GREEN, "maxBodySize: %lu", baseConfig.maxBodySize);
        }
      }
    } else if (trimmedLine.find("maxConnections") == 0) {
      size_t maxConnections;
      if (parseNumericValue(trimmedLine, "maxConnections", 14,
                            maxConnections)) {
        baseConfig.maxConnections = maxConnections;
        debuglog(GREEN, "maxConnections: %zu", baseConfig.maxConnections);
      }
    } else if (trimmedLine.find("requestTimeout") == 0) {
      int requestTimeout;
      if (parseNumericValue(trimmedLine, "requestTimeout", 14,
                            requestTimeout)) {
        baseConfig.requestTimeout = requestTimeout;
        debuglog(GREEN, "requestTimeout: %d", baseConfig.requestTimeout);
      }
    } else if (trimmedLine.find("responseTimeout") == 0) {
      int responseTimeout;
      if (parseNumericValue(trimmedLine, "responseTimeout", 15,
                            responseTimeout)) {
        baseConfig.responseTimeout = responseTimeout;
        debuglog(GREEN, "responseTimeout: %d", baseConfig.responseTimeout);
      }
    } else if (trimmedLine.find("keepalive_timeout") == 0) {
      int keepaliveTimeout;
      if (parseNumericValue(trimmedLine, "keepalive_timeout", 17,
                            keepaliveTimeout)) {
        baseConfig.keepalive_timeout = keepaliveTimeout;
        debuglog(GREEN, "keepalive_timeout: %d", baseConfig.keepalive_timeout);
      }
    } else if (trimmedLine.find("autoindex") == 0 && !baseConfig.parsedindex) {
      baseConfig.parsedindex = true;
      size_t valueStart =
          trimmedLine.find_first_not_of(" \t", 9); // Skip "autoindex"
      size_t valueEnd = trimmedLine.find(';', valueStart);

      int counts = 0;
      debuglog(GREEN, "autoindex: %s , time: %d", trimmedLine.c_str(),
               ++counts);

      std::string autoindexValue;
      if (valueEnd != std::string::npos) {
        autoindexValue = trimmedLine.substr(valueStart, valueEnd - valueStart);
      } else {
        autoindexValue = trimmedLine.substr(valueStart);
      }

      // Trim trailing whitespace
      autoindexValue =
          autoindexValue.substr(0, autoindexValue.find_last_not_of(" \t") + 1);

      if (autoindexValue == "on") {
        baseConfig.autoindex = true;
        debuglog(GREEN, "autoindex: on");
      } else if (autoindexValue == "off") {
        baseConfig.autoindex = false;
        debuglog(GREEN, "autoindex: off");
      } else {
        debuglog(YELLOW, "Warning: Invalid autoindex value: %s",
                 autoindexValue.c_str());
      }
    }

    else if (trimmedLine.find("error_page") == 0 &&
             trimmedLine.find("{") != std::string::npos) {
      // Handle error_page block
      size_t blockStart = trimmedLine.find("{");
      // Find the position of this line in the content
      size_t linePos = httpContent.find(trimmedLine);
      if (linePos != std::string::npos) {
        // Find the closing brace using proper brace counting
        size_t blockEnd =
            findClosingBrace(httpContent, linePos + blockStart + 1);

        if (blockEnd != std::string::npos) {
          std::string errorPageBlock = httpContent.substr(
              linePos + blockStart + 1, blockEnd - (linePos + blockStart + 1));
          parseErrorPageBlock(errorPageBlock, baseConfig);
        }
      }
    }
  }
  debuglog(GREEN, "Parsed global settings\n\n");
}

void Config::parseErrorPageBlock(const std::string &blockContent,
                                 BaseConf &baseConfig) {
  std::istringstream iss(blockContent);
  std::string line;

  while (std::getline(iss, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
      continue;
    size_t end = line.find_last_not_of(" \t\n\r");
    std::string trimmedLine = line.substr(start, end - start + 1);

    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue;
    }

    // Extract the first part as the error code (3-digit number)
    size_t spacePos = trimmedLine.find_first_of(" \t");
    if (spacePos == std::string::npos)
      continue;

    // Get the error code (should be 3 digits)
    std::string errorCodeStr = trimmedLine.substr(0, spacePos);
    if (errorCodeStr.length() != 3 || !isdigit(errorCodeStr[0]) ||
        !isdigit(errorCodeStr[1]) || !isdigit(errorCodeStr[2])) {
      debuglog(RED, "Invalid error code format: %s", errorCodeStr.c_str());
      continue;
    }

    int errorCode = atoi(errorCodeStr.c_str());

    // Extract the path (everything after the space until semicolon)
    size_t pathStart = trimmedLine.find_first_not_of(" \t", spacePos);
    size_t pathEnd = trimmedLine.find(';', pathStart);
    if (pathStart == std::string::npos)
      continue;

    std::string path;
    if (pathEnd != std::string::npos) {
      path = trimmedLine.substr(pathStart, pathEnd - pathStart);
    } else {
      path = trimmedLine.substr(pathStart);
    }

    // Remove any trailing whitespace from path
    path = path.substr(0, path.find_last_not_of(" \t") + 1);

    baseConfig.error_pages[errorCode] = path;
    debuglog(GREEN, "error page %d: %s", errorCode, path.c_str());
  }
}

void Config::parseServerBlocks(const std::string &httpContent,
                               HttpConfig &httpConfig, BaseConf &baseConfig) {
  // Look for server blocks in the content
  size_t pos = 0;
  int serverBlockCount = 0;

  while (true) {
    // Find the next server block
    pos = httpContent.find("server", pos);
    if (pos == std::string::npos) {
      if (serverBlockCount == 0) {
        throw std::runtime_error("No server blocks found in configuration");
      }
      break; // Exit the loop when no more server blocks are found
    }

    // Make sure this is actually a server block start (not a substring in a
    // comment)
    size_t lineStart = httpContent.rfind('\n', pos);
    if (lineStart == std::string::npos)
      lineStart = 0;

    // Check if there's anything other than whitespace before "server" on this
    // line
    std::string beforeServer = httpContent.substr(lineStart, pos - lineStart);
    if (beforeServer.find_first_not_of(" \t\n\r") != std::string::npos) {
      // This "server" is part of something else, continue searching
      pos += 6; // Skip "server"
      continue;
    }

    // Find opening brace
    size_t openBrace = httpContent.find("{", pos);
    if (openBrace == std::string::npos)
      break;

    // Check if there's only whitespace between "server" and "{"
    std::string between = httpContent.substr(pos + 6, openBrace - (pos + 6));
    if (between.find_first_not_of(" \t\n\r") != std::string::npos) {
      // Not a valid server block, continue searching
      pos = pos + 6;
      continue;
    }

    // Find the end of this server block using brace counting
    size_t blockStart = openBrace + 1;
    size_t blockEnd = findClosingBrace(httpContent, blockStart);

    if (blockEnd == std::string::npos) {
      throw std::runtime_error("Unclosed server block");
      break;
    }

    // Extract the server block content
    std::string serverBlockContent =
        httpContent.substr(blockStart, blockEnd - blockStart);

    // Parse this server block (create a new ServerData)

    ServerData ServerData;

    // Copy base settings first
    static_cast<BaseConf &>(ServerData) = baseConfig;
    // ServerData.upload_dir = baseConfig.upload_dir;
    ServerData.acceptedMethods = baseConfig.acceptedMethods;
    // debuglog(GREEN, "ServerData initialized with base settings, before server
    // parsing\n\n\n");

    parseServerBlock(serverBlockContent, ServerData);

    // Add the parsed server to the config
    httpConfig.servers.push_back(ServerData);

    // Move past this server block for the next iteration
    pos = blockEnd + 1;

    debuglog(GREEN, "Parsed server block %d\n\n", ++serverBlockCount);
  }
}

void Config::parseServerBlock(const std::string &serverBlockContent,
                              ServerData &ServerData) {
  std::istringstream iss(serverBlockContent);
  std::string line;

  while (std::getline(iss, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
      continue; // Skip empty lines
    size_t end = line.find_last_not_of(" \t\n\r");
    std::string trimmedLine = line.substr(start, end - start + 1);

    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue; // Skip comments
    }

    if (trimmedLine.find("serverListenAddress") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 19);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        ServerData.serverListenAddress = value;
      }
    } else if (trimmedLine.find("root") == 0 && !ServerData.parsedroot) {
      ServerData.parsedroot = true;
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        ServerData.root = value;
        if (ServerData.root[ServerData.root.length() - 1] != '/') {
          ServerData.root += '/';
        }
        ServerData.upload_dir = ServerData.root + "uploads/";
        debuglog(GREEN, "Server root: %s", ServerData.root.c_str());
        debuglog(GREEN, "Server upload_dir: %s", ServerData.upload_dir.c_str());
      }
    } else if (trimmedLine.find("listen") == 0) {
      size_t portStart =
          trimmedLine.find_first_not_of(" \t", 6); // Skip "listen"
      size_t portEnd = trimmedLine.find(';', portStart);
      if (portEnd != std::string::npos) {
        std::string portStr =
            trimmedLine.substr(portStart, portEnd - portStart);
        uint16_t port = static_cast<uint16_t>(atoi(portStr.c_str()));

        if (port > 0 && port <= 65535) {
          ServerData.ports.push_back(port);
          debuglog(GREEN, "Server listening on port: %u", port);
        } else {
          debuglog(RED, "Invalid port number: %s", portStr.c_str());
        }
      }
    } else if (trimmedLine.find("server_name") == 0) {
      size_t nameStart =
          trimmedLine.find_first_not_of(" \t", 11); // Skip "server_name"
      size_t nameEnd = trimmedLine.find(';', nameStart);

      if (nameEnd != std::string::npos) {
        // Extract the entire string containing server names
        std::string serverNames =
            trimmedLine.substr(nameStart, nameEnd - nameStart);

        // Use a string stream to split by whitespace
        std::istringstream nameStream(serverNames);
        std::string name;

        // Read each name separated by whitespace
        while (nameStream >> name) {
          ServerData.server_names.push_back(name);
          debuglog(GREEN, "Server name: %s", name.c_str());
        }
      }
    } else if (trimmedLine.find("index") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 5);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value == "on") {
          ServerData.autoindex = true;
          debuglog(GREEN, "Server autoindex: on");
        }
      }
    }

    else if (trimmedLine.find("limit_accept") == 0) {
      size_t methodsStart = trimmedLine.find_first_not_of(" \t", 12);
      size_t openBrace = trimmedLine.find("{");
      size_t semiColon = trimmedLine.find(";");

      // Check for the "limit_accept METHOD1 METHOD2 { ... }" format
      if (methodsStart != std::string::npos && openBrace != std::string::npos) {
        std::string methods =
            trimmedLine.substr(methodsStart, openBrace - methodsStart);
        std::istringstream methodStream(methods);
        std::string method;

        // Parse allowed methods
        while (methodStream >> method) {
          if (method != "{") {
            ServerData.acceptedMethods.push_back(method);
            debuglog(GREEN, "Server accepted method: %s", method.c_str());
          }
        }
      }
      // Check for the "accepted_methods METHOD1 METHOD2 METHOD3;" format
      else if (methodsStart != std::string::npos &&
               semiColon != std::string::npos) {
        std::string methods =
            trimmedLine.substr(methodsStart, semiColon - methodsStart);
        std::istringstream methodStream(methods);
        std::string method;

        // Parse allowed methods
        while (methodStream >> method) {
          ServerData.acceptedMethods.push_back(method);
          debuglog(GREEN, "Server accepted method: %s", method.c_str());
        }
      }
    }

    // Check for location blocks
    else if (trimmedLine.find("location") == 0) {
      // Explicitly set the has_locations flag
      ServerData.has_locations = true;
      debuglog(GREEN, "Config: Setting has_locations flag to true for server with port %u",
               ServerData.ports.empty() ? 0 : ServerData.ports[0]);
      
      // Find the path (between "location" and "{")
      size_t pathStart =
          trimmedLine.find_first_not_of(" \t", 8); // Skip "location"
      if (pathStart != std::string::npos) {
        size_t pathEnd;
        size_t openBrace = trimmedLine.find("{");

        if (openBrace != std::string::npos) {
          // Path ends at the brace or last non-whitespace
          pathEnd = trimmedLine.find_last_not_of(" \t", openBrace - 1);

          // Extract the path
          std::string path =
              trimmedLine.substr(pathStart, pathEnd - pathStart + 1);
          debuglog(GREEN, "Found location block for path: %s", path.c_str());

          // Find location block content in the serverBlockContent
          size_t locationPos = serverBlockContent.find(trimmedLine);

          if (locationPos != std::string::npos) {
            size_t blockStart = serverBlockContent.find("{", locationPos) + 1;
            size_t blockEnd = findClosingBrace(serverBlockContent, blockStart);

            if (blockEnd != std::string::npos) {
              // Extract location block content
              std::string locationContent =
                  serverBlockContent.substr(blockStart, blockEnd - blockStart);

              // Create a Location object
              Location location;

              // Parse location directives
              parseLocationBlock(locationContent, location, ServerData);

              // Add this location to the server data
              ServerData.location_blocks[path] = location;
              debuglog(GREEN, "Added location '%s' to server config, now has %lu locations",
                       path.c_str(), ServerData.location_blocks.size());
            }
          }
        }
      }
    }
    
    // ...existing code...
  }
  
  // Double check the has_locations flag before finishing
  if (!ServerData.location_blocks.empty() && !ServerData.has_locations) {
    ServerData.has_locations = true;
    debuglog(RED, "Config: Fixed has_locations flag for server with port %u, has %lu locations",
             ServerData.ports.empty() ? 0 : ServerData.ports[0],
             ServerData.location_blocks.size());
  }
}

void Config::parseLocationBlock(const std::string &locationContent,
                                Location &location, ServerData &ServerData) {
  std::istringstream iss(locationContent);
  std::string line;

  location.upload_dir = ServerData.upload_dir;
  location.acceptedMethods = ServerData.acceptedMethods;
  location.root = ServerData.root;
  location.autoindex = ServerData.autoindex;
  location.error_pages = ServerData.error_pages;

  while (std::getline(iss, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
      continue; // Skip empty lines
    size_t end = line.find_last_not_of(" \t\n\r");
    std::string trimmedLine = line.substr(start, end - start + 1);

    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue; // Skip comments
    }

    // Parse location directives
    if (trimmedLine.find("autoindex") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 9);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value == "on") {
          location.autoindex = true;
          debuglog(GREEN, "Location autoindex: on");
        } else if (value == "off") {
          location.autoindex = false;
          debuglog(GREEN, "Location autoindex: off");
        } else {
          debuglog(YELLOW, "Warning: Invalid autoindex value: %s",
                   value.c_str());
        }
      }
    } else if (trimmedLine.find("internal") == 0) {
      // Set the internal flag to true
      location.internal = true;
      debuglog(GREEN, "Location internal: true");
    } else if (trimmedLine.find("return") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 6);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string redirectStr =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        std::istringstream redirectStream(redirectStr);

        int code;
        std::string url;
        redirectStream >> code;

        // Get the rest as URL (may contain spaces)
        std::getline(redirectStream, url);
        url = url.substr(url.find_first_not_of(" \t"));

        location.return_directive = std::make_pair(code, url);
        debuglog(GREEN, "Location return: %d %s", code, url.c_str());
      }
    } else if (trimmedLine.find("root") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
      size_t valueEnd = trimmedLine.find(';', valueStart);
      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value[value.length() - 1] != '/') {
          value += '/';
        }
        location.root = value;
        location.upload_dir = value + "uploads/";
        debuglog(GREEN, "Location root: %s", value.c_str());
      }
    }

    else if (trimmedLine.find("file_upload") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 11);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value == "on") {
          location.file_upload = true;
          debuglog(GREEN, "Location file_upload: on");
        }
      }
    } else if (trimmedLine.find("upload_dir") == 0 ||
               trimmedLine.find("file_upload_path") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(
          " \t", trimmedLine.find("upload_dir") == 0 ? 10 : 16);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value[value.length() - 1] != '/') {
          value += '/';
        }
        location.upload_dir = value;
        debuglog(GREEN, "Location upload_dir: %s", value.c_str());
      }
    } else if (trimmedLine.find("acceptedMethods") == 0 ||
               trimmedLine.find("accepted_methods") == 0) {
      size_t methodsStart = trimmedLine.find_first_not_of(
          " \t", trimmedLine.find("acceptedMethods") == 0 ? 15 : 16);
      size_t openBrace = trimmedLine.find("{");
      size_t semiColon = trimmedLine.find(";");

      // Check for the "acceptedMethods METHOD1 METHOD2 { ... }" format
      if (methodsStart != std::string::npos && openBrace != std::string::npos) {
        std::string methods =
            trimmedLine.substr(methodsStart, openBrace - methodsStart);
        std::istringstream methodStream(methods);
        std::string method;

        // Parse allowed methods
        if (location.acceptedMethods.size() > 0)
          location.acceptedMethods.clear();
        while (methodStream >> method) {
          if (method != "{") {
            location.acceptedMethods.push_back(method);
            debuglog(GREEN, "Location accepted method: %s", method.c_str());
          }
        }
      }
      // Check for the "accepted_methods METHOD1 METHOD2 METHOD3;" format
      else if (methodsStart != std::string::npos &&
               semiColon != std::string::npos) {
        std::string methods =
            trimmedLine.substr(methodsStart, semiColon - methodsStart);
        std::istringstream methodStream(methods);
        std::string method;

        // Parse allowed methods
        while (methodStream >> method) {
          location.acceptedMethods.push_back(method);
          debuglog(GREEN, "Location accepted method: %s", method.c_str());
        }
      }
    }
  }
}

void Config::parseCgiBlock(const std::string &cgiContent, CGIData &cgiConfig) {
  std::istringstream iss(cgiContent);
  std::string line;

  while (std::getline(iss, line)) {
    // Trim whitespace
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
      continue; // Skip empty lines
    size_t end = line.find_last_not_of(" \t\n\r");
    std::string trimmedLine = line.substr(start, end - start + 1);

    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue; // Skip comments
    }

    // Parse CGI directives
    if (trimmedLine.find("cgi_path_alias") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 14);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      // Handle strings without semicolons (as in your example)
      std::string value;
      if (valueEnd != std::string::npos) {
        value = trimmedLine.substr(valueStart, valueEnd - valueStart);
      } else {
        value = trimmedLine.substr(valueStart);
      }

      std::istringstream pathStream(value);
      std::string path, alias;

      pathStream >> path >> alias;
      // Remove quotes if present
      if (!alias.empty() && alias[0] == '"' && alias[alias.size() - 1] == '"') {
        alias = alias.substr(1, alias.size() - 2);
      }

      // Store as a pair
      cgiConfig.cgi_path_alias = std::make_pair(path, alias);
      debuglog(GREEN, "CGI path alias: %s -> %s", path.c_str(), alias.c_str());
    } else if (trimmedLine.find("upload_dir") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 10);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      // Handle strings without semicolons
      std::string value;
      if (valueEnd != std::string::npos) {
        value = trimmedLine.substr(valueStart, valueEnd - valueStart);
      } else {
        value = trimmedLine.substr(valueStart);
      }

      cgiConfig.upload_dir = value;
      debuglog(GREEN, "CGI upload_dir: %s", value.c_str());
    } else if (trimmedLine.find("file_extension") == 0) {
      size_t valueStart = trimmedLine.find_first_not_of(" \t", 14);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      // Handle strings without semicolons
      std::string value;
      if (valueEnd != std::string::npos) {
        value = trimmedLine.substr(valueStart, valueEnd - valueStart);
      } else {
        value = trimmedLine.substr(valueStart);
      }

      // Split by whitespace to get individual extensions
      std::istringstream extStream(value);
      std::string ext;

      while (extStream >> ext) {
        cgiConfig.cgi_extensions.push_back(ext);
        debuglog(GREEN, "CGI file extension: %s", ext.c_str());
      }
    } else if (trimmedLine.find("acceptedMethods") == 0) {
      size_t methodsStart =
          trimmedLine.find_first_not_of(" \t", 15); // Skip "acceptedMethods"
      size_t openBrace = trimmedLine.find("{");

      if (methodsStart != std::string::npos) {
        // Extract the part between "acceptedMethods" and "{"
        std::string methodsStr;
        if (openBrace != std::string::npos) {
          methodsStr =
              trimmedLine.substr(methodsStart, openBrace - methodsStart);
        } else {
          // No opening brace on this line - take the rest of the string
          methodsStr = trimmedLine.substr(methodsStart);
        }

        // Trim trailing whitespace from the methods string
        size_t lastNonSpace = methodsStr.find_last_not_of(" \t\n\r");
        if (lastNonSpace != std::string::npos) {
          methodsStr = methodsStr.substr(0, lastNonSpace + 1);
        }

        // Split the methods string by whitespace
        std::istringstream methodStream(methodsStr);
        std::string method;

        // Clear existing methods
        if (cgiConfig.acceptedMethods.size() > 0)
          cgiConfig.acceptedMethods.clear();

        // Add each method to the vector
        while (methodStream >> method) {
          // Make sure it's not a brace
          if (method != "{" && method != "}") {
            cgiConfig.acceptedMethods.push_back(method);
            debuglog(GREEN, "CGI acceptedMethods method: %s", method.c_str());
          }
        }
      }
    }
  }
}

void Config::removeDuplicatePorts() {
  for (size_t i = 0; i < servers.size(); ++i) {
    for (size_t j = 0; j < servers[i].ports.size(); ++j) {
      for (size_t k = i + 1; k < servers.size(); ++k) {
        for (size_t l = 0; l < servers[k].ports.size(); ++l) {
          if (servers[i].ports[j] == servers[k].ports[l]) {
            // remove ports[l] from servers[k]
            servers[k].ports.erase(servers[k].ports.begin() + l);
            debuglog(RED, "remove duplicate port %ld from server[%d] %s",
                     servers[i].ports[j], k + 1,
                     servers[k].server_names[0].c_str());
          }
          if (servers[k].ports.size() == 0) {
            debuglog(RED, "remove server[%ld] %s because no port left", k + 1,
                     servers[k].server_names[0].c_str());
            servers.erase(servers.begin() + k);
            --k;
            break;
          }
        }
      }
    }
  }
}

void Config::configValidate() {
  if (servers.empty()) {
    debuglog(RED, "Configuration error: No servers specified");
    throw std::runtime_error("Invalid configuration");
  }
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i].ports.empty()) {
      debuglog(RED, "Configuration error: No ports specified");
      throw std::runtime_error("Invalid configuration");
    }
    if (servers[i].server_names.empty()) {
      debuglog(RED, "Configuration error: No server names specified");
      throw std::runtime_error("Invalid configuration");
    }
  }
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

// void Config::debugprintConfigs()
// {
//     debuglog(BLUE, "==== Configuration Summary ====");
//     debuglog(BLUE, "Total servers: %zu", servers.size());

//     for (size_t i = 0; i < servers.size(); ++i) {
//         const ServerData& server = servers[i];

//         debuglog(BLUE, "\n----- Server %zu -----", i + 1);

//         // Print ports
//         if (!server.ports.empty()) {
//             std::string portList;
//             for (size_t j = 0; j < server.ports.size(); ++j) {
//                 if (j > 0) portList += ", ";
//                 char portStr[8];
//                 snprintf(portStr, sizeof(portStr), "%hu", server.ports[j]);
//                 portList += portStr;
//             }
//             debuglog(BLUE, "Ports: %s", portList.c_str());
//         } else {
//             debuglog(BLUE, "Ports: None defined");
//         }

//         // Print server names
//         if (!server.server_names.empty()) {
//             std::string nameList;
//             for (size_t j = 0; j < server.server_names.size(); ++j) {
//                 if (j > 0) nameList += ", ";
//                 nameList += server.server_names[j];
//             }
//             debuglog(BLUE, "Server Names: %s", nameList.c_str());
//         } else {
//             debuglog(BLUE, "Server Names: None defined");
//         }

//         // Print basic server settings
//         debuglog(BLUE, "Root: %s", server.root.c_str());
//         debuglog(BLUE, "Index: %s", server.index.c_str());
//         debuglog(BLUE, "Max Body Size: %lu bytes", server.maxBodySize);
//         debuglog(BLUE, "Request Timeout: %d seconds", server.requestTimeout);
//         debuglog(BLUE, "Response Timeout: %d seconds",
//         server.responseTimeout); debuglog(BLUE, "Keepalive Timeout: %d
//         seconds", server.keepalive_timeout); debuglog(BLUE, "Autoindex: %s",
//         server.autoindex ? "on" : "off"); debuglog(BLUE, "File Server: %s",
//         server.file_server ? "on" : "off"); debuglog(BLUE, "Upload Directory:
//         %s", server.upload_dir.c_str());

//         // Print error pages
//         if (!server.error_pages.empty()) {
//             debuglog(BLUE, "Error Pages:");
//             std::map<int, std::string>::const_iterator it;
//             for (it = server.error_pages.begin(); it !=
//             server.error_pages.end(); ++it) {
//                 debuglog(BLUE, "  %d: %s", it->first, it->second.c_str());
//             }
//         }

//         // Print location blocks
//         if (server.has_locations && !server.location_blocks.empty()) {
//             debuglog(BLUE, "Location Blocks (%zu):",
//             server.location_blocks.size()); std::map<std::string,
//             Location>::const_iterator it; for (it =
//             server.location_blocks.begin(); it !=
//             server.location_blocks.end(); ++it) {
//                 const std::string& path = it->first;
//                 const Location& loc = it->second;

//                 debuglog(BLUE, "  Location: %s", path.c_str());

//                 // Only print non-default values for clarity
//                 debuglog(BLUE, "    Autoindex: %s", loc.autoindex ? "on" :
//                 "off"); debuglog(BLUE, "    File Upload: %s", loc.file_upload
//                 ? "on" : "off"); debuglog(BLUE, "    Upload Dir: %s",
//                 loc.upload_dir.c_str());

//                 debuglog(BLUE, "    root : %s", loc.root.c_str());
//                // if (!loc.alias.empty()) debuglog(BLUE, "    Alias: %s",
//                loc.alias.c_str());
//                 if (loc.internal) debuglog(BLUE, "    Internal: %d",
//                 loc.internal); else
//                     debuglog(BLUE, "    Internal is not set\n ");
//                      if (loc.return_directive.first != 0) {

//                         debuglog(BLUE, "    Return: %d %s",
//                         loc.return_directive.first,
//                         loc.return_directive.second.c_str());
//                     }

//                 if (!loc.acceptedMethods.empty()) {
//                     std::string methods;
//                     for (size_t j = 0; j < loc.acceptedMethods.size(); ++j) {
//                         if (j > 0) methods += " ";
//                         methods += loc.acceptedMethods[j];
//                     }
//                     debuglog(BLUE, "    Accepted Methods: %s",
//                     methods.c_str());
//                 }
//                 //if(!loc.index.empty()) {
//                    // debuglog(BLUE, "    Index: %s", loc.index.c_str());
//                // }
//                     debuglog(BLUE, " PRINTING ERROR PAGES");
//                     std::map<int, std::string>::const_iterator it;
//                     for (it = loc.error_pages.begin(); it !=
//                     loc.error_pages.end(); ++it) {
//                         debuglog(BLUE, "    Error Page: %d %s", it->first,
//                         it->second.c_str());
//                     }
//             }
//         }

//         // Print CGI settings
//         if (server.cgi_exists) {
//             debuglog(BLUE, "CGI Settings:");
//             debuglog(BLUE, "  Path Alias: %s -> %s",
//                   server.cgiData.cgi_path_alias.first.c_str(),
//                   server.cgiData.cgi_path_alias.second.c_str());
//             debuglog(BLUE, "  Upload Dir: %s",
//             server.cgiData.upload_dir.c_str());

//             if (!server.cgiData.cgi_extensions.empty()) {
//                 std::string exts;
//                 for (size_t j = 0; j < server.cgiData.cgi_extensions.size();
//                 ++j) {
//                     if (j > 0) exts += " ";
//                     exts += server.cgiData.cgi_extensions[j];
//                 }
//                 debuglog(BLUE, "  File Extensions: %s", exts.c_str());
//             }

//             if (!server.cgiData.acceptedMethods.empty()) {
//                 std::string methods;
//                 for (size_t j = 0; j < server.cgiData.acceptedMethods.size();
//                 ++j) {
//                     if (j > 0) methods += " ";
//                     methods += server.cgiData.acceptedMethods[j];
//                 }
//                 debuglog(BLUE, "  Limited HTTP Methods: %s",
//                 methods.c_str());

//             }

//         }
//     }

//      // Print port mapping
//      debuglog(BLUE, "\nPort Mapping:");

//      debuglog(BLUE, "\nPort Mapping:");
//      std::map<uint16_t, ServerData*>::const_iterator it;
//      for (it = port_map_.begin(); it != port_map_.end(); ++it) {
//          size_t serverIndex = it->second - &servers[0];  // Calculate server
//          index debuglog(BLUE, "  Port %hu: Server %zu", it->first,
//          serverIndex);
//      }

//     debuglog(BLUE, "\n==== End of Configuration Summary ====");
// }

// /**
//  * @brief Prints detailed information about a single ServerData structure
//  *
//  * @param ServerData The ServerData structure to print
//  */
// void Config::debugprintServerData(const ServerData& ServerData)
// {
//     debuglog(YELLOW, "======= Server Configuration Details =======");

//     // Print basic server configuration
//     debuglog(YELLOW, "Server base configuration:");
//     debuglog(YELLOW, "  maxBodySize: %lu", ServerData.maxBodySize);
//     debuglog(YELLOW, "  maxConnections: %lu", ServerData.maxConnections);
//     debuglog(YELLOW, "  requestTimeout: %d", ServerData.requestTimeout);
//     debuglog(YELLOW, "  responseTimeout: %d", ServerData.responseTimeout);
//     debuglog(YELLOW, "  keepalive_timeout: %d",
//     ServerData.keepalive_timeout); debuglog(YELLOW, "  autoindex: %s",
//     ServerData.autoindex ? "true" : "false"); debuglog(YELLOW, " file_server:
//     %s", ServerData.file_server ? "true" : "false"); debuglog(YELLOW, "
//     upload_dir: %s", ServerData.upload_dir.c_str()); debuglog(YELLOW, "
//     serverListenAddress: %s", ServerData.serverListenAddress.c_str());
//     debuglog(YELLOW, "  root: %s", ServerData.root.c_str());
//     debuglog(YELLOW, "  index: %s", ServerData.index.c_str());

//     // Print ports
//     if (!ServerData.ports.empty()) {
//         debuglog(YELLOW, "Server ports (%zu):", ServerData.ports.size());
//         for (size_t i = 0; i < ServerData.ports.size(); ++i) {
//             debuglog(YELLOW, "  %zu: %hu", i + 1, ServerData.ports[i]);
//         }
//     } else {
//         debuglog(YELLOW, "No server ports defined");
//     }

//     // Print server names
//     if (!ServerData.server_names.empty()) {
//         debuglog(YELLOW, "Server names (%zu):",
//         ServerData.server_names.size()); for (size_t i = 0; i <
//         ServerData.server_names.size(); ++i) {
//             debuglog(YELLOW, "  %zu: %s", i + 1,
//             ServerData.server_names[i].c_str());
//         }
//     } else {
//         debuglog(YELLOW, "No server names defined");
//     }

//     // Print accepted methods
//     if (!ServerData.acceptedMethods.empty()) {
//         debuglog(YELLOW, "Server accepted methods (%zu):",
//         ServerData.acceptedMethods.size()); for (size_t i = 0; i <
//         ServerData.acceptedMethods.size(); ++i) {
//             debuglog(YELLOW, "  %zu: %s", i + 1,
//             ServerData.acceptedMethods[i].c_str());
//         }
//     } else {
//         debuglog(YELLOW, "No server accepted methods defined");
//     }

//     // Print error pages
//     if (!ServerData.error_pages.empty()) {
//         debuglog(YELLOW, "Server error pages (%zu):",
//         ServerData.error_pages.size()); std::map<int,
//         std::string>::const_iterator it; for (it =
//         ServerData.error_pages.begin(); it != ServerData.error_pages.end();
//         ++it) {
//             debuglog(YELLOW, "  %d: %s", it->first, it->second.c_str());
//         }
//     } else {
//         debuglog(YELLOW, "No server error pages defined");
//     }

//     // Print CGI configuration
//     debuglog(YELLOW, "Server cgi_exists: %s", ServerData.cgi_exists ? "true"
//     : "false");

//     if (ServerData.cgi_exists) {
//         debuglog(YELLOW, "CGI Configuration:");
//         debuglog(YELLOW, "  cgi_path_alias: %s -> %s",
//                 ServerData.cgiData.cgi_path_alias.first.c_str(),
//                 ServerData.cgiData.cgi_path_alias.second.c_str());
//         debuglog(YELLOW, "  upload_dir: %s",
//         ServerData.cgiData.upload_dir.c_str());

//         // Print file extensions
//         if (!ServerData.cgiData.cgi_extensions.empty()) {
//             debuglog(YELLOW, "  cgi_extensions (%zu):",
//             ServerData.cgiData.cgi_extensions.size()); for (size_t i = 0; i <
//             ServerData.cgiData.cgi_extensions.size(); ++i) {
//                 debuglog(YELLOW, "    %zu: %s", i + 1,
//                 ServerData.cgiData.cgi_extensions[i].c_str());
//             }
//         } else {
//             debuglog(YELLOW, "  No file extensions defined");
//         }

//         // Print acceptedMethods methods
//         if (!ServerData.cgiData.acceptedMethods.empty()) {
//             debuglog(YELLOW, "  acceptedMethods methods (%zu):",
//             ServerData.cgiData.acceptedMethods.size()); for (size_t i = 0; i
//             < ServerData.cgiData.acceptedMethods.size(); ++i) {
//                 debuglog(YELLOW, "    %zu: %s", i + 1,
//                 ServerData.cgiData.acceptedMethods[i].c_str());
//             }
//         } else {
//             debuglog(YELLOW, "  No acceptedMethods methods defined");
//         }

//     }

//     // Print location blocks
//     debuglog(YELLOW, "Server has_locations: %s", ServerData.has_locations ?
//     "true" : "false");

//     if (ServerData.has_locations && !ServerData.location_blocks.empty()) {
//         debuglog(YELLOW, "Location Blocks (%zu):",
//         ServerData.location_blocks.size());

//         std::map<std::string, Location>::const_iterator it;
//         for (it = ServerData.location_blocks.begin(); it !=
//         ServerData.location_blocks.end(); ++it) {
//             const std::string& path = it->first;
//             const Location& loc = it->second;

//             debuglog(YELLOW, "  Location path: %s", path.c_str());
//             debuglog(YELLOW, "    autoindex: %s", loc.autoindex ? "true" :
//             "false"); debuglog(YELLOW, "    file_upload: %s", loc.file_upload
//             ? "true" : "false"); debuglog(YELLOW, "    upload_dir: %s",
//             loc.upload_dir.c_str());

//            // if (!loc.alias.empty()) {
//                // debuglog(YELLOW, "    alias: %s", loc.alias.c_str());
//             //}

//             //if (!loc.index.empty()) {
//                 //debuglog(YELLOW, "    index: %s", loc.index.c_str());
//            // }

//             debuglog(YELLOW, "    internal: %s", loc.internal ? "true" :
//             "false");

//             if (loc.return_directive.first != 0) {
//                 debuglog(YELLOW, "    return: %d %s",
//                       loc.return_directive.first,
//                       loc.return_directive.second.c_str());
//             }

//             // Print location's accepted methods
//             if (!loc.acceptedMethods.empty()) {
//                 debuglog(YELLOW, "    accepted methods (%zu):",
//                 loc.acceptedMethods.size()); for (size_t i = 0; i <
//                 loc.acceptedMethods.size(); ++i) {
//                     debuglog(YELLOW, "      %zu: %s", i + 1,
//                     loc.acceptedMethods[i].c_str());
//                 }
//             } else {
//                 debuglog(YELLOW, "    No accepted methods defined");
//             }

//             // Print location's error pages
//             if (!loc.error_pages.empty()) {
//                 debuglog(YELLOW, "    error pages (%zu):",
//                 loc.error_pages.size()); std::map<int,
//                 std::string>::const_iterator ep; for (ep =
//                 loc.error_pages.begin(); ep != loc.error_pages.end(); ++ep) {
//                     debuglog(YELLOW, "      %d: %s", ep->first,
//                     ep->second.c_str());
//                 }
//             }
//         }
//     } else {
//         debuglog(YELLOW, "No location blocks defined");
//     }

//     debuglog(YELLOW, "========== End of Server Data ==========\n");
// }