#include <stdexcept>
#include "Config.hpp"
#include "ServerData.hpp"
#include "debug.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

/** Initialize the static variables */
std::vector<ServerData> Config::configs_;
Config *Config::instance_ = NULL;
std::string Config::_filename = "config/default.conf"; // Default filename
std::map<uint16_t, ServerData *> Config::port_map_;

template<typename T>
bool parseNumericValue(const std::string& line, const std::string& param, size_t paramLen, T& outValue) {
    size_t valueStart = line.find_first_not_of(" \t", paramLen);
    size_t valueEnd = line.find(';', valueStart);
    
    if (valueEnd != std::string::npos) {
        std::string valueStr = line.substr(valueStart, valueEnd - valueStart);
        outValue = static_cast<T>(atoi(valueStr.c_str()));
        return true;
    }
    return false;
}

size_t findClosingBrace(const std::string& content, size_t start)
{
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

void Config::parseGlobalSettings(const std::string& httpContent, BaseConf &baseConfig) 
{
    std::istringstream iss(httpContent);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue; // Skip empty lines
        size_t end = line.find_last_not_of(" \t\n\r");
        std::string trimmedLine = line.substr(start, end - start + 1);
        
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue; // Skip comments
        }
        
        // Check for global settings
        if (trimmedLine.find("maxBodySize") == 0) 
        {
            size_t maxBodySize;
            if (parseNumericValue(trimmedLine, "maxBodySize", 11, maxBodySize)) {
            if (maxBodySize <= 0) {
                debuglog(YELLOW, "Warning: Invalid maxBodySize value: %lu, using default", maxBodySize);
                      // Keep default value
            } else {
                    baseConfig.maxBodySize = maxBodySize;
                    debuglog(GREEN, "maxBodySize: %lu", baseConfig.maxBodySize);
                }
            }
        }
        else if (trimmedLine.find("maxConnections") == 0) {
          size_t maxConnections;
          if (parseNumericValue(trimmedLine, "maxConnections", 14, maxConnections)) {
              baseConfig.maxConnections = maxConnections;
              debuglog(GREEN, "maxConnections: %zu", baseConfig.maxConnections);
          }
      }
      else if (trimmedLine.find("requestTimeout") == 0) {
        int requestTimeout;
        if (parseNumericValue(trimmedLine, "requestTimeout", 14, requestTimeout)) {
            baseConfig.requestTimeout = requestTimeout;
            debuglog(GREEN, "requestTimeout: %d", baseConfig.requestTimeout);
        }
     }
     else if (trimmedLine.find("responseTimeout") == 0) {
        int responseTimeout;
        if (parseNumericValue(trimmedLine, "responseTimeout", 15, responseTimeout)) {
            baseConfig.responseTimeout = responseTimeout;
            debuglog(GREEN, "responseTimeout: %d", baseConfig.responseTimeout);
        }
     }
      else if (trimmedLine.find("keepalive_timeout") == 0) {
        int keepaliveTimeout;
        if (parseNumericValue(trimmedLine, "keepalive_timeout", 17, keepaliveTimeout)) {
            baseConfig.keepalive_timeout = keepaliveTimeout;
            debuglog(GREEN, "keepalive_timeout: %d", baseConfig.keepalive_timeout);
        }
        }
        // Add more global settings as needed
        else if (trimmedLine.find("error_page") == 0 && trimmedLine.find("{") != std::string::npos) {
          // Handle error_page block
          size_t blockStart = trimmedLine.find("{");
          // Find the position of this line in the content
          size_t linePos = httpContent.find(trimmedLine);
          if (linePos != std::string::npos) {
              // Find the closing brace using proper brace counting
              size_t blockEnd = findClosingBrace(httpContent, linePos + blockStart + 1);
              
              if (blockEnd != std::string::npos) {
                  std::string errorPageBlock = httpContent.substr(linePos + blockStart + 1, 
                                                                 blockEnd - (linePos + blockStart + 1));
                  parseErrorPageBlock(errorPageBlock, baseConfig);
              }
          }
      }
    }
}


void Config::parseErrorPageBlock(const std::string& blockContent, BaseConf& baseConfig)
{
    std::istringstream iss(blockContent);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\n\r");
        std::string trimmedLine = line.substr(start, end - start + 1);
        
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue;
        }
        
        // Extract the first part as the error code (3-digit number)
        size_t spacePos = trimmedLine.find_first_of(" \t");
        if (spacePos == std::string::npos) continue;
        
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
        if (pathStart == std::string::npos) continue;
        
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

void Config::parseServerBlocks(const std::string& httpContent, HttpConfig& httpConfig, BaseConf& baseConfig)
{
    // Look for server blocks in the content
    size_t pos = 0;
    int serverBlockCount = 0;
    
    while (true) {
        // Find the next server block
        pos = httpContent.find("server", pos);
        if (pos == std::string::npos) break;
        
        // Make sure this is actually a server block start (not a substring in a comment)
        size_t lineStart = httpContent.rfind('\n', pos);
        if (lineStart == std::string::npos) lineStart = 0;
        
        // Check if there's anything other than whitespace before "server" on this line
        std::string beforeServer = httpContent.substr(lineStart, pos - lineStart);
        if (beforeServer.find_first_not_of(" \t\n\r") != std::string::npos) {
            // This "server" is part of something else, continue searching
            pos += 6; // Skip "server"
            continue;
        }
        
        // Find opening brace
        size_t openBrace = httpContent.find("{", pos);
        if (openBrace == std::string::npos) break;
        
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
            debuglog(RED, "Error: Unclosed server block");
            break;
        }
        
        // Extract the server block content
        std::string serverBlockContent = httpContent.substr(blockStart, blockEnd - blockStart);
        
        // Parse this server block (create a new ServerData)
        ServerData serverData;
        // Copy base settings first
        static_cast<BaseConf&>(serverData) = baseConfig;
        // Copy global settings to server data
        // static_cast<BaseConf&>(serverData) = baseConfig;
        
        // Parse the server block content
        parseServerBlock(serverBlockContent, serverData);
        
        
        // Add the parsed server to the config
        httpConfig.servers.push_back(serverData);
        
        // Move past this server block for the next iteration
        pos = blockEnd + 1;
        
        debuglog(GREEN, "Parsed server block %d", ++serverBlockCount);
    }
}

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

void Config::parseServerBlock(const std::string& serverBlockContent, ServerData& serverData)
{
    std::istringstream iss(serverBlockContent);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue; // Skip empty lines
        size_t end = line.find_last_not_of(" \t\n\r");
        std::string trimmedLine = line.substr(start, end - start + 1);
        
        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue; // Skip comments
        }
        
        if (trimmedLine.find("serverListenAddress") == 0) {
          size_t valueStart = trimmedLine.find_first_not_of(" \t", 19);
          size_t valueEnd = trimmedLine.find(';', valueStart);
          
          if (valueEnd != std::string::npos) {
              std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
              serverData.serverListenAddress = value;
              debuglog(GREEN, "Server root: %s", serverData.root.c_str());
          }
      }
        else if (trimmedLine.find("listen") == 0) {
            size_t portStart = trimmedLine.find_first_not_of(" \t", 6); // Skip "listen"
            size_t portEnd = trimmedLine.find(';', portStart);
            if (portEnd != std::string::npos) {
                std::string portStr = trimmedLine.substr(portStart, portEnd - portStart);
                uint16_t port = static_cast<uint16_t>(atoi(portStr.c_str()));
                
                if (port > 0 && port <= 65535) {
                    serverData.ports.push_back(port);
                    debuglog(GREEN, "Server listening on port: %u", port);
                } else {
                    debuglog(RED, "Invalid port number: %s", portStr.c_str());
                }
            }
        }
        else if (trimmedLine.find("server_name") == 0) {
            size_t nameStart = trimmedLine.find_first_not_of(" \t", 12); // Skip "server_name"
            size_t nameEnd = trimmedLine.find(';', nameStart);
            if (nameEnd != std::string::npos) {
                std::string serverName = trimmedLine.substr(nameStart, nameEnd - nameStart);
                serverData.server_names.push_back(serverName);
                debuglog(GREEN, "Server name: %s", serverName.c_str());
            }
        }
        else if (trimmedLine.find("index") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 5);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                if (value == "on") {
                    serverData.autoindex = true;
                    debuglog(GREEN, "Server autoindex: on");
                }
            }
        }    
        else if (trimmedLine.find("root") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                serverData.root = value;
                debuglog(GREEN, "Server root: %s", serverData.root.c_str());
            }
        }
        // Check for location blocks
        else if (trimmedLine.find("location") == 0) {
          has_locations = true;
            // Find the path (between "location" and "{")
            size_t pathStart = trimmedLine.find_first_not_of(" \t", 8); // Skip "location"
            if (pathStart != std::string::npos) {
                size_t pathEnd;
                size_t openBrace = trimmedLine.find("{");
                
                if (openBrace != std::string::npos) {
                    // Path ends at the brace or last non-whitespace
                    pathEnd = trimmedLine.find_last_not_of(" \t", openBrace - 1);
                    
                    // Extract the path
                    std::string path = trimmedLine.substr(pathStart, pathEnd - pathStart + 1);
                    debuglog(GREEN, "Found location block for path: %s", path.c_str());
                    
                    // Find location block content in the serverBlockContent
                    size_t locationPos = serverBlockContent.find(trimmedLine);
                    
                    if (locationPos != std::string::npos) {
                        size_t blockStart = serverBlockContent.find("{", locationPos) + 1;
                        size_t blockEnd = findClosingBrace(serverBlockContent, blockStart);
                        
                        if (blockEnd != std::string::npos) {
                            // Extract location block content
                            std::string locationContent = serverBlockContent.substr(
                                blockStart, blockEnd - blockStart);
                                
                            // Create a Location object
                            Location location;
                            
                            // Parse location directives
                            parseLocationBlock(locationContent, location);
                            
                            // Add this location to the server data
                            serverData.location_blocks[path] = location;
                        }
                    }
                }
              }
              else if (trimmedLine.find("cgi") == 0) {
                  size_t openBrace = trimmedLine.find("{");
                  cgi_exists = true;
                  
                  if (openBrace != std::string::npos) {
                      // Find CGI block content
                      size_t locationPos = serverBlockContent.find(trimmedLine);
                      
                      if (locationPos != std::string::npos) {
                          size_t blockStart = serverBlockContent.find("{", locationPos) + 1;
                          size_t blockEnd = findClosingBrace(serverBlockContent, blockStart);
                          
                          if (blockEnd != std::string::npos) {
                              // Extract CGI block content
                              std::string cgiContent = serverBlockContent.substr(
                                  blockStart, blockEnd - blockStart);
                                  
                              // Parse CGI directives
                              parseCgiBlock(cgiContent, serverData.cgiData);
                          }
            }
        }
    }
}
    }
  }

void Config::parseLocationBlock(const std::string& locationContent, Location& location)
{
    std::istringstream iss(locationContent);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue; // Skip empty lines
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
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                if (value == "on") {
                    location.autoindex = true;
                    debuglog(GREEN, "Location autoindex: on");
                }
            }
        }
        else if (trimmedLine.find("index") == 0) {
            // Note: Your Location struct doesn't have an index field,
            // you might want to add one or handle it differently
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 5);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                // location.index = value; // Uncomment if you add index field
                debuglog(GREEN, "Location index: %s", value.c_str());
            }
        }
        else if (trimmedLine.find("root") == 0) {
            // Note: Your Location struct doesn't have a root field,
            // you might want to add one or handle it differently
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                // location.root = value; // Uncomment if you add root field
                debuglog(GREEN, "Location root: %s", value.c_str());
            }
        }
        else if (trimmedLine.find("alias") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 5);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                location.alias = value;
                debuglog(GREEN, "Location alias: %s", value.c_str());
            }
        }
        else if (trimmedLine.find("internal") == 0) {
            // Set the internal flag to true
            location.internal = true;
            debuglog(GREEN, "Location internal: true");
        }
        else if (trimmedLine.find("return") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 6);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string redirectStr = trimmedLine.substr(valueStart, valueEnd - valueStart);
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
        }
        else if (trimmedLine.find("file_upload") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 11);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                if (value == "on") {
                    location.file_upload = true;
                    debuglog(GREEN, "Location file_upload: on");
                }
            }
        }
        else if (trimmedLine.find("upload_dir") == 0 || trimmedLine.find("file_upload_path") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", trimmedLine.find("upload_dir") == 0 ? 10 : 16);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
                location.upload_dir = value;
                debuglog(GREEN, "Location upload_dir: %s", value.c_str());
            }
        }
        else if (trimmedLine.find("error_page") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 10);
            size_t valueEnd = trimmedLine.find(';', valueStart);
            
            if (valueEnd != std::string::npos) {
                std::string errorStr = trimmedLine.substr(valueStart, valueEnd - valueStart);
                std::istringstream errorStream(errorStr);
                
                int code;
                std::string path;
                errorStream >> code >> path;
                
                location.error_pages[code] = path;
                debuglog(GREEN, "Location error_page: %d %s", code, path.c_str());
            }
        }
        else if (trimmedLine.find("limit_except") == 0 || trimmedLine.find("accepted_methods") == 0) {
            size_t methodsStart = trimmedLine.find_first_not_of(" \t", trimmedLine.find("limit_except") == 0 ? 12 : 16);
            size_t openBrace = trimmedLine.find("{");
            size_t semiColon = trimmedLine.find(";");
            
            // Check for the "limit_except METHOD1 METHOD2 { ... }" format
            if (methodsStart != std::string::npos && openBrace != std::string::npos) {
                std::string methods = trimmedLine.substr(methodsStart, openBrace - methodsStart);
                std::istringstream methodStream(methods);
                std::string method;
                
                // Parse allowed methods
                while (methodStream >> method) {
                    if (method != "{") {
                        location.acceptedMethods.push_back(method);
                        debuglog(GREEN, "Location accepted method: %s", method.c_str());
                    }
                }
            }
            // Check for the "accepted_methods METHOD1 METHOD2 METHOD3;" format
            else if (methodsStart != std::string::npos && semiColon != std::string::npos) {
                std::string methods = trimmedLine.substr(methodsStart, semiColon - methodsStart);
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

void Config::parseCgiBlock(const std::string& cgiContent, CGIData& cgiConfig)
{
    std::istringstream iss(cgiContent);
    std::string line;
    
    while (std::getline(iss, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue; // Skip empty lines
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
            // Store as a pair instead of a map entry
            cgiConfig.cgi_path_alias = std::make_pair(path, alias);
            debuglog(GREEN, "CGI path alias: %s -> %s", path.c_str(), alias.c_str());
        }
        else if (trimmedLine.find("upload_dir") == 0) {
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
        }
        else
        {
            // Handle other CGI directives as needed
            debuglog(YELLOW, "Unknown CGI directive: %s", trimmedLine.c_str());
        }
    }
}

/**
 * @brief Constructor for the Config class
 *
 * It will get a filename and read the configuration file
 * or default to a default configuration file if none is provided.
 *
 * For now it is hardcoded.alignas
 * The configuration according to the subject is an array of servers
 */

 Config::Config(std::string filename)
 {
      _filename = filename;
     std::ifstream configFile(filename.c_str());
     if (!configFile.is_open()) {
         throw std::runtime_error("Failed to open config file: " + filename);
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
     
     size_t httpEnd = findClosingBrace(content, httpStart + 6); // +6 to skip "http {"
     if (httpEnd == std::string::npos) {
         throw std::runtime_error("Unclosed http block in configuration");
     }
     
     // Extract HTTP block content
     std::string httpContent = content.substr(httpStart + 6, httpEnd - (httpStart + 6));
     
     // Parse global settings first
     parseGlobalSettings(httpContent, baseConfig);

     // Parse server blocks
     parseServerBlocks(httpContent, httpConfig, baseConfig);
     
     // Copy the parsed servers to the static configs_ vector
     configs_ = httpConfig.servers;
     
     debuglog(GREEN, "Config initialized with %zu servers", configs_.size());
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

std::vector<ServerData> &Config::getServerData(char *config_file) {
  if (instance_ == NULL) {
    instance_ = new Config(Config::_filename);
  }
  return Config::configs_;
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




/*
Config::Config(std::string filename) {
  (void)filename;

  // First server configuration
  ServerData server1;
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
  /*
  server1.server_names.push_back("myWebserver");
  server1.server_names.push_back("someWebserver");
  server1.root = "www";
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

  server1.upload_dir = "www/uploads";
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
  server2.root = "./www/html";

  // Third server configuration
  ServerData server3;
  server3.ports.push_back(4247);
  server3.server_names.push_back("myWebserver");
  server3.server_names.push_back("someWebserver");
  server3.root = "./www/html";

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
}*/
