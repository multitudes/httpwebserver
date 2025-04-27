
#include "Parser.hpp"
#include "debug.h"
#include <set>
#include <string>
#include <map>
#include <vector>

using std::vector;
using std::string;
using std::map;
using std::stringstream;
using std::ifstream;

namespace Parser {

  long starttime = 0;

void parse(std::string filename, std::vector<ServerData> &servers,
           std::map<uint16_t, ServerData *> &port_map_) {

  servers.clear();
  port_map_.clear();

  long starttime = getCurrentTimeMillis();
  debuglog(GREEN, "Parsing configuration file at time: %ld", starttime);

  std::string content = OpenReadConfigFile(filename);
  std::string httpContent = abstratHttpContent(content);
  std::string globalContent = extractGlobalConfig(httpContent);
  BaseConf baseConfig;

  parseGlobalSettings(globalContent, baseConfig);
  parseServerBlocks(httpContent, servers, baseConfig);
  parsePortToServer(servers, port_map_);

  //debugprintConfigs(servers, port_map_);
  return;
}

std::string OpenReadConfigFile(std::string filename)
{
  std::ifstream configFile(filename.c_str());
  if (!configFile.is_open()) {
    throw std::runtime_error("Failed to open config file: " + filename);
  }

  // Read entire file into a string
  stringstream buffer;
  buffer << configFile.rdbuf();
  string content = buffer.str();
  configFile.close();
  return content;
}

std::string abstratHttpContent(std::string content)
{
  size_t httpStart = content.find("http {");
  if (httpStart == string::npos) {
    throw std::runtime_error("No http block found in configuration");
  }

  size_t httpEnd =
      findClosingBrace(content, httpStart + 6); // +6 to skip "http {"
  if (httpEnd == string::npos) {
    throw std::runtime_error("Unclosed http block in configuration");
  }
  // Extract HTTP block content
  string httpContent =
      content.substr(httpStart + 6, httpEnd - (httpStart + 6));
      return httpContent; 
}

std::string extractGlobalConfig(const std::string &httpContent) {
  std::istringstream iss(httpContent);
  std::string line;
  std::string globalConfig;
  
  while (std::getline(iss, line)) {
      std::string trimmedLine = trimLine(line);
      if (trimmedLine.empty() || trimmedLine[0] == '#')
          continue;
          
      // Check if we've found the first server block
      if (trimmedLine.find("server") != std::string::npos && 
          trimmedLine.find("{") != std::string::npos) {
          debuglog(GREEN, "Found first server block, ended global config extraction");
          break; // Stop extraction when server block is found
      }
      globalConfig += line + "\n";
  }

  return globalConfig;
}

void parsePortToServer(std::vector<ServerData> &servers,
  std::map<uint16_t, ServerData *> &port_map_) {

    for (size_t i = 0; i < servers.size(); ++i) {
        for (size_t j = 0; j < servers[i].ports.size(); ++j) {
              port_map_[servers[i].ports[j]] = &servers[i];
        }
    }
}

void parseGlobalSettings(const std::string &globalContent, BaseConf &baseConfig) {

  std::istringstream iss(globalContent);
  std::string line;

  while (std::getline(iss, line)) {
    std::string trimmedLine = trimLine(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#') {
        continue; // Skip comments
    }

    if (trimmedLine.find("maxBodySize") == 0) {
        parseMaxBodySize(trimmedLine, baseConfig);
    } 
    else if (trimmedLine.find("autoindex") == 0) {
        parseAutoIndex(trimmedLine, baseConfig);
    }
    else if(trimmedLine.find("error_pages") == 0 && trimmedLine.find("{") != std::string::npos) {
          std::string errorPageBlock = abstractErrorPageBlock(trimmedLine, globalContent, baseConfig);
          parseErrorPageBlock(errorPageBlock, baseConfig);
        }
      }
      debuglog(GREEN, "Parsed global settings");
  }

void parseMaxBodySize(std::string &trimmedLine, BaseConf &baseConfig)
{
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
}

void parseAutoIndex(std::string &trimmedLine, BaseConf &baseConfig){
  
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 9);
  size_t valueEnd = trimmedLine.find(';', valueStart);

  std::string autoindexValue;
  if (valueEnd != std::string::npos) {
    autoindexValue = trimmedLine.substr(valueStart, valueEnd - valueStart);
  } 
  else {
    autoindexValue = trimmedLine.substr(valueStart);
  }
  autoindexValue = autoindexValue.substr(0, autoindexValue.find_last_not_of(" \t") + 1);

  switch (getAutoindexCode(autoindexValue)) {
    case 1: // "on"
        baseConfig.autoindex = true;
        debuglog(GREEN, "autoindex: on");
        break;
        
    case 0: // "off" 
        baseConfig.autoindex = false;
        debuglog(GREEN, "autoindex: off");
        break;
        
    default:
        debuglog(YELLOW, "Warning: Invalid autoindex value: %s",
                 autoindexValue.c_str());
        break;
  }
}

int getAutoindexCode(const std::string &value) {
  if (value == "on") return 1;
  if (value == "off") return 0;
  return -1; // invalid value
}

std::string abstractErrorPageBlock(std::string &trimmedLine, const std::string &httpContent, 
  BaseConf &baseConfig){
    size_t blockStart = trimmedLine.find("{");
    size_t linePos = httpContent.find(trimmedLine);
    std::string errorPageBlock; // Declare errorPageBlock here
    if (linePos != std::string::npos) {
        size_t blockEnd = findClosingBrace(httpContent, linePos + blockStart + 1);
        if (blockEnd != std::string::npos) {
            errorPageBlock = httpContent.substr(
              linePos + blockStart + 1, blockEnd - (linePos + blockStart + 1)); 
        }
    }
    return errorPageBlock;
}


void parseErrorPageBlock(const std::string &blockContent,
                         BaseConf &baseConfig) {
  std::istringstream iss(blockContent);
  string line;

  while (std::getline(iss, line)) {
    std::string trimmedLine = trimLine(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue;
    }
    // Extract the first part as the error code (3-digit number)
    size_t spacePos = trimmedLine.find_first_of(" \t");
    if (spacePos == string::npos)
      continue;

    // Get the error code (should be 3 digits)
    string errorCodeStr = trimmedLine.substr(0, spacePos);
    if (errorCodeStr.length() != 3 || !isdigit(errorCodeStr[0]) ||
        !isdigit(errorCodeStr[1]) || !isdigit(errorCodeStr[2])) {
      debuglog(RED, "Invalid error code format: %s", errorCodeStr.c_str());
      continue;
    }

    int errorCode = atoi(errorCodeStr.c_str());
    std::string path = extractPathFromLine(trimmedLine, spacePos);

    baseConfig.error_pages[errorCode] = path;
    debuglog(GREEN, "error page %d: %s", errorCode, path.c_str());
  }
}

std::string extractPathFromLine(const std::string &trimmedLine, size_t spacePos) {
  // Find the start of the path
  size_t pathStart = trimmedLine.find_first_not_of(" \t", spacePos);
  size_t pathEnd = trimmedLine.find(';', pathStart);
  if (pathStart == std::string::npos)
    return ""; // No path found

  std::string path;
  if (pathEnd != std::string::npos) {
    path = trimmedLine.substr(pathStart, pathEnd - pathStart);
  } else {
    path = trimmedLine.substr(pathStart);
  }
  return path;
}

void parseServerBlocks(const std::string &httpContent,
  std::vector<ServerData> &servers, BaseConf &baseConfig) {
  size_t pos = 0;
  int serverBlockCount = 0;
  std::set<int> PortSet;

  while (true) {
    // Find and extract next server block
    ServerBlockInfo blockInfo;
    if (!findNextServerBlock(httpContent, pos, blockInfo)) {
        if (serverBlockCount == 0) {
            throw std::runtime_error("No server blocks found in configuration");
      }
      break;
    }

    ServerData serverData;
    static_cast<BaseConf &>(serverData) = baseConfig;
    serverData.acceptedMethods = baseConfig.acceptedMethods;

    parseServerBlock(blockInfo.content, serverData, PortSet);
    addServerIfValid(servers, serverData, serverBlockCount);

// Move past this server block
    pos = blockInfo.endPos + 1;
    ++serverBlockCount;
    debuglog(GREEN, "Parsed server block %d\n\n", serverBlockCount);
  }
}

bool findNextServerBlock(const std::string &httpContent, size_t pos, 
  ServerBlockInfo &blockInfo) {
    // Find the next server block
    pos = httpContent.find("server", pos);
    if (pos == std::string::npos)
      return false;

    // Verify server keyword is at start of line
    if (!isValidServerKeyword(httpContent, pos))
        return findNextServerBlock(httpContent, pos + 6, blockInfo);

    // Find and verify opening brace
    size_t openBrace = httpContent.find("{", pos);
    if (openBrace == std::string::npos || !isOnlyWhitespace(httpContent.substr(pos + 6, openBrace - (pos + 6))))
        return findNextServerBlock(httpContent, pos + 6, blockInfo);

    // Find closing brace
    size_t blockStart = openBrace + 1;
    size_t blockEnd = findClosingBrace(httpContent, blockStart);

    if (blockEnd == std::string::npos)
        throw std::runtime_error("Unclosed server block");

    blockInfo.content = httpContent.substr(blockStart, blockEnd - blockStart);
    blockInfo.startPos = blockStart;
    blockInfo.endPos = blockEnd;
    return true;
}

bool isValidServerKeyword(const std::string &content, size_t pos) {
  size_t lineStart = content.rfind('\n', pos);
  if (lineStart == std::string::npos)
    lineStart = 0;
    
  std::string beforeServer = content.substr(lineStart, pos - lineStart);
  return beforeServer.find_first_not_of(" \t\n\r") == std::string::npos;
}

bool isOnlyWhitespace(const std::string &str) {
  return str.find_first_not_of(" \t\n\r") == std::string::npos;
}

void addServerIfValid(std::vector<ServerData> &servers, ServerData &serverData, int serverBlockCount) {
  if (serverData.ports.empty()) {
    debuglog(GREEN, "No ports found in server block %d, ignore server %d", serverBlockCount, serverBlockCount);
  } else {
    // Valid server with at least one port
    servers.push_back(serverData);
    debuglog(GREEN, "Added server with %zu ports", serverData.ports.size());
  }
}


void parseServerBlock(const std::string &serverBlockContent,
  ServerData &serverData, std::set<int> &portset) {
  std::istringstream iss(serverBlockContent);
  string line;

  while (std::getline(iss, line)) {
    std::string trimmedLine = trimLine(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue; 
    }

    if (trimmedLine.find("serverListenAddress") == 0) 
      parseServerListenAddress(trimmedLine, serverData);
    else if (trimmedLine.find("root") == 0 && !serverData.parsedroot) 
      parseServerRoot(trimmedLine, serverData);
    else if (trimmedLine.find("listen") == 0)
      parseServerPort(trimmedLine, serverData, portset);
    else if (trimmedLine.find("server_name") == 0)
      parseServerName(trimmedLine, serverData);
    else if (trimmedLine.find("index") == 0)
      parseServerIndax(trimmedLine, serverData);
    else if (trimmedLine.find("acceptedMethods") == 0)
      parseAccceptedMethods(trimmedLine, serverData);
    else if (trimmedLine.find("location") == 0)
      parseLocationBlocks(serverBlockContent, trimmedLine, serverData);
    else if (trimmedLine.find("cgi") == 0)
      parseCgiConfig(trimmedLine, serverBlockContent, serverData);
  }
}

void parseServerListenAddress(const std::string &trimmedLine, ServerData &serverData){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 19);
  size_t valueEnd = trimmedLine.find(';', valueStart);

  if (valueEnd != std::string::npos) {
    std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
    serverData.serverListenAddress = value;
  }
}

void parseServerRoot(std::string trimmedLine, ServerData &serverData){

  serverData.parsedroot = true;
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
  size_t valueEnd = trimmedLine.find(';', valueStart);

  if (valueEnd != std::string::npos) {
    std::string value =
        trimmedLine.substr(valueStart, valueEnd - valueStart);
    serverData.root = value;
    if (serverData.root[serverData.root.length() - 1] != '/') {
      serverData.root += '/';
    }
    serverData.upload_dir = serverData.root + "upload/";
    debuglog(GREEN, "Server root: %s", serverData.root.c_str());
    debuglog(GREEN, "Server upload_dir: %s", serverData.upload_dir.c_str());
  }
}
    
void parseServerPort(std::string trimmedLine, ServerData &serverData, std::set<int> &portset){
  size_t portStart = trimmedLine.find_first_not_of(" \t", 6);
  size_t portEnd = trimmedLine.find(';', portStart);
  if (portEnd != std::string::npos) {
    std::string portStr =
        trimmedLine.substr(portStart, portEnd - portStart);
    uint16_t port = static_cast<uint16_t>(atoi(portStr.c_str()));

    if (port > 0 && port <= 65535) {
      if (portset.find(port) != portset.end()) {
        debuglog(GREEN, "Port %u is duplicated", port);
      } else {
        portset.insert(port);
        serverData.ports.push_back(port);
        debuglog(GREEN, "Server listening on port: %u", port);
      }

    } 
    else {
      debuglog(RED, "Invalid port number: %s", portStr.c_str());
    }
  }
}

void parseServerName(std::string &trimmedLine, ServerData &serverData){
  size_t nameStart = trimmedLine.find_first_not_of(" \t", 11);
      size_t nameEnd = trimmedLine.find(';', nameStart);

      if (nameEnd != string::npos) {
        string serverNames =
            trimmedLine.substr(nameStart, nameEnd - nameStart);

        std::istringstream nameStream(serverNames);
        string name;

        // Read each name separated by whitespace
        while (nameStream >> name) {
          serverData.server_names.push_back(name);
          debuglog(GREEN, "Server name: %s", name.c_str());
        }
      }
}

void parseServerIndax(std::string &trimmedLine, ServerData &serverData){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 5);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != string::npos) {
        string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value == "on") {
          serverData.autoindex = true;
          debuglog(GREEN, "Server autoindex: on");
        }
      }
}

void parseAccceptedMethods(std::string &trimmedLine, ServerData &serverData){
  size_t methodsStart = trimmedLine.find_first_not_of(" \t", 12);
      size_t openBrace = trimmedLine.find("{");
      size_t semiColon = trimmedLine.find(";");

    if (methodsStart != std::string::npos && openBrace != std::string::npos) {
      std::string methods =
          trimmedLine.substr(methodsStart, openBrace - methodsStart);
      std::istringstream methodStream(methods);
      std::string method;

      while ((methodStream >> method) && (method != "{")){
          serverData.acceptedMethods.push_back(method);
          debuglog(GREEN, "Server accepted method: %s", method.c_str());
      }
    }
    else if (methodsStart != std::string::npos && semiColon != std::string::npos) {
      std::string methods =
          trimmedLine.substr(methodsStart, semiColon - methodsStart);
      std::istringstream methodStream(methods);
      std::string method;

      while (methodStream >> method) {
        serverData.acceptedMethods.push_back(method);
        debuglog(GREEN, "Server accepted method: %s", method.c_str());
      }
    }
}

void parseLocationBlocks(const std::string &serverBlockContent, const std::string &trimmedLine, 
      ServerData &serverData) {
  serverData.has_locations = true;

  std::string path = extractLocationPath(trimmedLine);
  if (path.empty()) {
    debuglog(RED, "Failed to extract valid location path");
    return;
  }
  
  debuglog(GREEN, "Found location block for path: %s", path.c_str());

  std::string locationContent = extractLocationContent(serverBlockContent, trimmedLine);
  if (locationContent.empty()) {
    debuglog(RED, "Failed to extract location content for path: %s", path.c_str());
    return;
  }
  
  Location location;
  parseLocationBlock(locationContent, location, serverData);
  serverData.location_blocks[path] = location;
}

std::string extractLocationPath(const std::string &trimmedLine) {
  
  size_t pathStart = trimmedLine.find_first_not_of(" \t", 8);
  if (pathStart == std::string::npos) {
    return "";
  }
 
  size_t openBrace = trimmedLine.find("{");
  if (openBrace == std::string::npos) {
    return "";
  }

  size_t pathEnd = trimmedLine.find_last_not_of(" \t", openBrace - 1);
  if (pathEnd == std::string::npos || pathEnd < pathStart) {
    return "";
  }

  return trimmedLine.substr(pathStart, pathEnd - pathStart + 1);
}

std::string extractLocationContent(const std::string &serverBlockContent,
   const std::string &trimmedLine) {
  
  size_t locationPos = serverBlockContent.find(trimmedLine);
  if (locationPos == std::string::npos) {
    return "";
  }

  size_t blockStart = serverBlockContent.find("{", locationPos);
  if (blockStart == std::string::npos) {
    return "";
  }
  blockStart++;
  
  size_t blockEnd = findClosingBrace(serverBlockContent, blockStart);
  if (blockEnd == std::string::npos) {
    return "";
  }
  
  return serverBlockContent.substr(blockStart, blockEnd - blockStart);
}

void parseLocationBlock(const std::string &locationContent, Location &location,
                        ServerData &serverData) {
  std::istringstream iss(locationContent);
  string line;

  initBasicVariables(location, serverData);
  
  while (std::getline(iss, line)) {
    std::string trimmedLine = trimLine(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue;
    }

    if (trimmedLine.find("autoindex") == 0)
      parseLocationAutoIndex(trimmedLine, location);
    else if (trimmedLine.find("internal") == 0)
        location.internal = true;
    else if (trimmedLine.find("return") == 0)
      parseLocationReturn(trimmedLine, location);
    else if (trimmedLine.find("root") == 0)
      parseLocationRoot(trimmedLine, location);
    else if (trimmedLine.find("file_upload") == 0)
      parseLocationFileUpload(trimmedLine, location);
    else if (trimmedLine.find("upload_dir") == 0)
      parseLocationUploadDir(trimmedLine, location);
    else if (trimmedLine.find("acceptedMethods") == 0)
      parseLocationAccceptedMethods(trimmedLine, location);
  }
}


void initBasicVariables(Location &location, ServerData &serverData){
  location.upload_dir = serverData.upload_dir;
  location.acceptedMethods = serverData.acceptedMethods;
  location.root = serverData.root;
  location.autoindex = serverData.autoindex;
  location.error_pages = serverData.error_pages;
}

void parseLocationAutoIndex(std::string &trimmedLine, Location &location){
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
}

void parseLocationReturn(std::string trimmedLine, Location &location){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 6);
  size_t valueEnd = trimmedLine.find(';', valueStart);

  if (valueEnd != std::string::npos) {
    std::string redirectStr = trimmedLine.substr(valueStart, valueEnd - valueStart);
    std::istringstream redirectStream(redirectStr);

    int code;
    std::string url;
    redirectStream >> code;

    std::getline(redirectStream, url);
    url = url.substr(url.find_first_not_of(" \t"));
    location.return_directive = std::make_pair(code, url);
    debuglog(GREEN, "Location return: %d %s", code, url.c_str());
    }
}

void parseLocationRoot(std::string trimmedLine, Location &location){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
  size_t valueEnd = trimmedLine.find(';', valueStart);
  if (valueEnd != std::string::npos) {
    std::string value =
        trimmedLine.substr(valueStart, valueEnd - valueStart);
    if (value[value.length() - 1] != '/') {
      value += '/';
    }
    location.root = value;
    location.upload_dir = value + "upload/";
    debuglog(GREEN, "Location root: %s", value.c_str());
  }
}

void parseLocationFileUpload(std::string &trimmedLine, Location &location) {
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 11);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != string::npos) {
        string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value == "on") {
          location.file_upload = true;
          debuglog(GREEN, "Location file_upload: on");
        }
      }
}

void parseLocationUploadDir(std::string trimmedLine, Location &location){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 10);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != string::npos) {
        string value =
            trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value[value.length() - 1] != '/') {
          value += '/';
        }
        location.upload_dir = value;
        debuglog(GREEN, "Location upload_dir: %s", value.c_str());
      }
}

void parseLocationAccceptedMethods(std::string &trimmedLine, Location &location){
  size_t methodsStart = trimmedLine.find_first_not_of(" \t", 15);
  size_t openBrace = trimmedLine.find("{");

  if (methodsStart != std::string::npos) {
    std::string methodsStr;
    if (openBrace != std::string::npos) {
      methodsStr =
          trimmedLine.substr(methodsStart, openBrace - methodsStart);
    } else {
      methodsStr = trimmedLine.substr(methodsStart);
    }

    size_t lastNonSpace = methodsStr.find_last_not_of(" \t\n\r");
    if (lastNonSpace != std::string::npos) {
      methodsStr = methodsStr.substr(0, lastNonSpace + 1);
    }

    std::istringstream methodStream(methodsStr);
    std::string method;

    if (location.acceptedMethods.size() > 0)
      location.acceptedMethods.clear();

    while ((methodStream >> method) && (method != "{" && method != "}")) {
        location.acceptedMethods.push_back(method);
        debuglog(GREEN, "location acceptedMethods method: %s", method.c_str());
    }
  }
}

void parseCgiConfig(const std::string &trimmedLine, const std::string &serverBlockContent, 
  ServerData &serverData) {

  serverData.cgi_exists = true;
  std::string cgiContent = extractCgiBlockContent(trimmedLine, serverBlockContent);

  if (!cgiContent.empty()) {
      parseCgiBlock(cgiContent, serverData.cgiData);
  }
}

std::string extractCgiBlockContent(const std::string &line, 
                const std::string &serverContent) {
  size_t openBrace = line.find("{");
  if (openBrace == std::string::npos) {
      return ""; 
  }

    size_t linePos = serverContent.find(line);
    if (linePos == std::string::npos) {
      return "";
    }

  size_t blockStart = serverContent.find("{", linePos);
  if (blockStart == std::string::npos) {
    return "";
  }
  blockStart++;

  size_t blockEnd = findClosingBrace(serverContent, blockStart);
  if (blockEnd == std::string::npos) {
    debuglog(RED, "Error: Unclosed CGI block");
    return "";
  }

  return serverContent.substr(blockStart, blockEnd - blockStart);
}

void parseCgiBlock(const std::string &cgiContent, CGIData &cgiConfig) {
  std::istringstream iss(cgiContent);
  string line;

  while (std::getline(iss, line)) {
    std::string trimmedLine = trimLine(line);
    if (trimmedLine.empty() || trimmedLine[0] == '#') {
      continue; // Skip comments
    }

    if (trimmedLine.find("cgi_path_alias") == 0)
      parseCgiPathAlias(trimmedLine, cgiConfig);
    else if (trimmedLine.find("upload_dir") == 0)
      parseCgiUploadDir(trimmedLine, cgiConfig);
    else if (trimmedLine.find("file_extension") == 0)
      parseCgiFileExtension(trimmedLine, cgiConfig);
    else if (trimmedLine.find("acceptedMethods") == 0)
      parseCGIAcceptedMethods(trimmedLine, cgiConfig);
  }
}

void parseCgiPathAlias(std::string &trimmedLine, CGIData &cgiConfig){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 14);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      string value;
      if (valueEnd != string::npos) {
        value = trimmedLine.substr(valueStart, valueEnd - valueStart);
      } else {
        value = trimmedLine.substr(valueStart);
      }

      std::istringstream pathStream(value);
      string path, alias;

      pathStream >> path >> alias;
      if (!alias.empty() && alias[0] == '"' && alias[alias.size() - 1] == '"') {
        alias = alias.substr(1, alias.size() - 2);
      }

      cgiConfig.cgi_path_alias = std::make_pair(path, alias);
      debuglog(GREEN, "CGI path alias: %s -> %s", path.c_str(), alias.c_str());
}

void parseCgiUploadDir(std::string &trimmedLine, CGIData &cgiConfig){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 10);
      size_t valueEnd = trimmedLine.find(';', valueStart);

      string value;
      if (valueEnd != string::npos) {
        value = trimmedLine.substr(valueStart, valueEnd - valueStart);
      } else {
        value = trimmedLine.substr(valueStart);
      }

      cgiConfig.upload_dir = value;
      debuglog(GREEN, "CGI upload_dir: %s", value.c_str());
}

void parseCgiFileExtension(std::string &trimmedLine, CGIData &cgiConfig){
  size_t valueStart = trimmedLine.find_first_not_of(" \t", 14);
  size_t valueEnd = trimmedLine.find(';', valueStart);

  std::string value;
  if (valueEnd != std::string::npos) {
    value = trimmedLine.substr(valueStart, valueEnd - valueStart);
  } else {
    value = trimmedLine.substr(valueStart);
  }

  std::istringstream extStream(value);
  std::string ext;

  while (extStream >> ext) {
    cgiConfig.cgi_extensions.push_back(ext);
    debuglog(GREEN, "CGI file extension: %s", ext.c_str());
  }
}

void parseCGIAcceptedMethods(std::string &trimmedLine,CGIData &cgiConfig){
  size_t methodsStart = trimmedLine.find_first_not_of(" \t", 15);
      size_t openBrace = trimmedLine.find("{");

    if (methodsStart != std::string::npos) {
      std::string methodsStr;
      if (openBrace != std::string::npos) {
        methodsStr = trimmedLine.substr(methodsStart, openBrace - methodsStart);
      } else {
        methodsStr = trimmedLine.substr(methodsStart);
      }

      size_t lastNonSpace = methodsStr.find_last_not_of(" \t\n\r");
      if (lastNonSpace != std::string::npos) {
        methodsStr = methodsStr.substr(0, lastNonSpace + 1);
      }

      std::istringstream methodStream(methodsStr);
      std::string method;

      if (cgiConfig.acceptedMethods.size() > 0)
        cgiConfig.acceptedMethods.clear();

      while ((methodStream >> method) && (method != "{" && method != "}")){
          cgiConfig.acceptedMethods.push_back(method);
          debuglog(GREEN, "CGI acceptedMethods method: %s", method.c_str());
      }
  }
}


size_t findClosingBrace(const string &content, size_t start) {
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
  return string::npos;
}

template <typename T>
bool parseNumericValue(const std::string &line, const std::string &param,
                       size_t paramLen, T &outValue) {
  (void)param; // TODO do we need ?
  size_t valueStart = line.find_first_not_of(" \t", paramLen);
  size_t valueEnd = line.find(';', valueStart);

  if (valueEnd != std::string::npos) {
    std::string valueStr = line.substr(valueStart, valueEnd - valueStart);
    outValue = static_cast<T>(atoi(valueStr.c_str()));
    return true;
  }
  return false;
}

} // namespace Parser
