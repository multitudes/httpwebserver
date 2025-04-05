
#include "debug.h"
#include "Parser.hpp"
#include <set>

namespace Parser {

    void parse(std::string filename, std::vector<ServerData>& servers, std::map<uint16_t, ServerData*> &port_map_) 
    {
        std::ifstream configFile(filename.c_str());
        if (!configFile.is_open()) {
                throw std::runtime_error("Failed to open config file: " + filename);
        }

        // Read entire file into a string
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        std::string content = buffer.str();
        configFile.close();

        BaseConf baseConfig;

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

        parseGlobalSettings(httpContent, baseConfig);
        parseServerBlocks(httpContent, servers, baseConfig);

        for (size_t i = 0; i < servers.size(); ++i) {
            for (size_t j = 0; j < servers[i].ports.size(); ++j) {
            port_map_[servers[i].ports[j]] = &servers[i];
            }
        }
        debugprintConfigs(servers, port_map_);
        return ;
    }

    void parseGlobalSettings(const std::string &httpContent, BaseConf &baseConfig) 
    {
        std::istringstream iss(httpContent);
        std::string line;

        while (std::getline(iss, line)) 
        {
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
                    debuglog(YELLOW, "Warning: Invalid maxBodySize value: %lu, using default",
                   maxBodySize);
            // Keep default value
            } 
            else {
                baseConfig.maxBodySize = maxBodySize;
                debuglog(GREEN, "maxBodySize: %lu", baseConfig.maxBodySize);
                }
            }
        }
        else if (trimmedLine.find("autoindex") == 0 && !baseConfig.parsedindex) {
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
            } 
            else {
                autoindexValue = trimmedLine.substr(valueStart);
            }

            // Trim trailing whitespace
            autoindexValue = autoindexValue.substr(0, autoindexValue.find_last_not_of(" \t") + 1);

            if (autoindexValue == "on") {
                baseConfig.autoindex = true;
                debuglog(GREEN, "autoindex: on");
            }
            else if (autoindexValue == "off") {
            baseConfig.autoindex = false;
            debuglog(GREEN, "autoindex: off");
            } 
            else {
                debuglog(YELLOW, "Warning: Invalid autoindex value: %s", autoindexValue.c_str());
            }
        }

        else if (trimmedLine.find("error_page") == 0 && trimmedLine.find("{") != std::string::npos) 
        {
            // Handle error_page block
            size_t blockStart = trimmedLine.find("{");
            // Find the position of this line in the content
            size_t linePos = httpContent.find(trimmedLine);
            if (linePos != std::string::npos) {
            // Find the closing brace using proper brace counting
                size_t blockEnd = findClosingBrace(httpContent, linePos + blockStart + 1);

                if (blockEnd != std::string::npos) {
                    std::string errorPageBlock = httpContent.substr( linePos + blockStart + 1, blockEnd - (linePos + blockStart + 1));
                    parseErrorPageBlock(errorPageBlock, baseConfig);
                }
            }
        }
  }
  debuglog(GREEN, "Parsed global settings\n\n");
}

void parseErrorPageBlock(const std::string &blockContent,
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
        } 
        else {
            path = trimmedLine.substr(pathStart);
        }

        // Remove any trailing whitespace from path
        path = path.substr(0, path.find_last_not_of(" \t") + 1);

        baseConfig.error_pages[errorCode] = path;
        debuglog(GREEN, "error page %d: %s", errorCode, path.c_str());
    }
}

void parseServerBlocks(const std::string &httpContent, 
            std::vector<ServerData>& servers, BaseConf &baseConfig) {
    // Look for server blocks in the content
    size_t pos = 0;
    int serverBlockCount = 0;
    std::set<int> PortSet;

    while (true) {
        // Find the next server block
        pos = httpContent.find("server", pos);
        if (pos == std::string::npos) {
            if (serverBlockCount == 0) {
                throw std::runtime_error("No server blocks found in configuration");
            }
        break; // Exit the loop when no more server blocks are found
        }

        size_t lineStart = httpContent.rfind('\n', pos);
        if (lineStart == std::string::npos)
            lineStart = 0;

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
        std::string serverBlockContent = httpContent.substr(blockStart, blockEnd - blockStart);

        // Parse this server block (create a new ServerData)
        ServerData ServerData;

        // Copy base settings first
        static_cast<BaseConf &>(ServerData) = baseConfig;
        ServerData.acceptedMethods = baseConfig.acceptedMethods;

        parseServerBlock(serverBlockContent, ServerData, PortSet);

        // Add the parsed server to the config
        if(ServerData.ports.empty()) {
            debuglog(RED, "No ports found in server block %d", serverBlockCount);
        }
        else {
            // Valid server with at least one port
            servers.push_back(ServerData);
            debuglog(GREEN, "Added server with %zu ports", ServerData.ports.size());
        }

        // Move past this server block for the next iteration
        pos = blockEnd + 1;
        debuglog(GREEN, "Parsed server block %d\n\n", ++serverBlockCount);
    }
}

void parseServerBlock(const std::string &serverBlockContent,
                              ServerData &ServerData, std::set<int> &Portset) {
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
            std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
            ServerData.serverListenAddress = value;
        }
    } 
    else if (trimmedLine.find("root") == 0 && !ServerData.parsedroot) 
    {
        ServerData.parsedroot = true;
        size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
        size_t valueEnd = trimmedLine.find(';', valueStart);

        if (valueEnd != std::string::npos) {
            std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
            ServerData.root = value;
            if (ServerData.root[ServerData.root.length() - 1] != '/') {
                ServerData.root += '/';
            }
            ServerData.upload_dir = ServerData.root + "uploads/";
            debuglog(GREEN, "Server root: %s", ServerData.root.c_str());
            debuglog(GREEN, "Server upload_dir: %s", ServerData.upload_dir.c_str());
        }
    } 
    else if (trimmedLine.find("listen") == 0) {
        size_t portStart = trimmedLine.find_first_not_of(" \t", 6);
        size_t portEnd = trimmedLine.find(';', portStart);
        if (portEnd != std::string::npos) {
            std::string portStr =
            trimmedLine.substr(portStart, portEnd - portStart);
            uint16_t port = static_cast<uint16_t>(atoi(portStr.c_str()));

            if (port > 0 && port <= 65535) {
                if(Portset.find(port) != Portset.end()) {
                    debuglog(RED, "Port %u is already in use", port);
                } 
                else {
                    Portset.insert(port);
                    ServerData.ports.push_back(port);
                    debuglog(GREEN, "Server listening on port: %u", port);
                }
          
            } 
            else {
                debuglog(RED, "Invalid port number: %s", portStr.c_str());
            }
        }
    } 
    else if (trimmedLine.find("server_name") == 0) {
        size_t nameStart = trimmedLine.find_first_not_of(" \t", 11);
        size_t nameEnd = trimmedLine.find(';', nameStart);

        if (nameEnd != std::string::npos) {
            std::string serverNames = trimmedLine.substr(nameStart, nameEnd - nameStart);

            std::istringstream nameStream(serverNames);
            std::string name;

            // Read each name separated by whitespace
            while (nameStream >> name) {
                ServerData.server_names.push_back(name);
                debuglog(GREEN, "Server name: %s", name.c_str());
            }
        }
    } 
    else if (trimmedLine.find("index") == 0) {
        size_t valueStart = trimmedLine.find_first_not_of(" \t", 5);
        size_t valueEnd = trimmedLine.find(';', valueStart);

        if (valueEnd != std::string::npos) {
            std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
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

        if (methodsStart != std::string::npos && openBrace != std::string::npos) {
            std::string methods = trimmedLine.substr(methodsStart, openBrace - methodsStart);
            std::istringstream methodStream(methods);
            std::string method;

            while (methodStream >> method) {
                if (method != "{") {
                    ServerData.acceptedMethods.push_back(method);
                    debuglog(GREEN, "Server accepted method: %s", method.c_str());
                }
            }
        }
        // Check for the "accepted_methods METHOD1 METHOD2 METHOD3;" format
        else if (methodsStart != std::string::npos && semiColon != std::string::npos) {
            std::string methods = trimmedLine.substr(methodsStart, semiColon - methodsStart);
            std::istringstream methodStream(methods);
            std::string method;

            while (methodStream >> method) {
                ServerData.acceptedMethods.push_back(method);
            debuglog(GREEN, "Server accepted method: %s", method.c_str());
            }
        }
    }
    else if (trimmedLine.find("location") == 0) {
        ServerData.has_locations = true;
        size_t pathStart = trimmedLine.find_first_not_of(" \t", 8);
        if (pathStart != std::string::npos) {
        size_t pathEnd;
        size_t openBrace = trimmedLine.find("{");

        if (openBrace != std::string::npos) {
            pathEnd = trimmedLine.find_last_not_of(" \t", openBrace - 1);

            std::string path = trimmedLine.substr(pathStart, pathEnd - pathStart + 1);
            debuglog(GREEN, "Found location block for path: %s", path.c_str());

            // Find location block content in the serverBlockContent
            size_t locationPos = serverBlockContent.find(trimmedLine);

            if (locationPos != std::string::npos) {
                size_t blockStart = serverBlockContent.find("{", locationPos) + 1;
                size_t blockEnd = findClosingBrace(serverBlockContent, blockStart);

                if (blockEnd != std::string::npos) {
                std::string locationContent = serverBlockContent.substr(blockStart, blockEnd - blockStart);

                Location location;
                parseLocationBlock(locationContent, location, ServerData);
                ServerData.location_blocks[path] = location;
            }
          }
        }
      }
    } 
    else if (trimmedLine.find("cgi") == 0) {
        size_t openBrace = trimmedLine.find("{");
        ServerData.cgi_exists = true;

        if (openBrace != std::string::npos) {
            size_t locationPos = serverBlockContent.find(trimmedLine);

            if (locationPos != std::string::npos) {
                size_t blockStart = serverBlockContent.find("{", locationPos) + 1;
                size_t blockEnd = findClosingBrace(serverBlockContent, blockStart);

                if (blockEnd != std::string::npos) {
                    std::string cgiContent = serverBlockContent.substr(blockStart, blockEnd - blockStart);
                parseCgiBlock(cgiContent, ServerData.cgiData);
                }
            }
        }
    }
  }
}

void parseLocationBlock(const std::string &locationContent,
                                Location &location, ServerData &ServerData) {
    std::istringstream iss(locationContent);
    std::string line;

    location.upload_dir = ServerData.upload_dir;
    location.acceptedMethods = ServerData.acceptedMethods;
    location.root = ServerData.root;
    location.autoindex = ServerData.autoindex;
    location.error_pages = ServerData.error_pages;

    while (std::getline(iss, line)) {
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
            std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
            if (value == "on") {
                location.autoindex = true;
                debuglog(GREEN, "Location autoindex: on");
            } 
            else if (value == "off") {
                location.autoindex = false;
                debuglog(GREEN, "Location autoindex: off");
            } 
            else {
                debuglog(YELLOW, "Warning: Invalid autoindex value: %s", value.c_str());
            }
        }
    } 
    else if (trimmedLine.find("internal") == 0) {
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

            std::getline(redirectStream, url);
            url = url.substr(url.find_first_not_of(" \t"));
            location.return_directive = std::make_pair(code, url);
            debuglog(GREEN, "Location return: %d %s", code, url.c_str());
        }
    } 
    else if (trimmedLine.find("root") == 0) {
        size_t valueStart = trimmedLine.find_first_not_of(" \t", 4);
        size_t valueEnd = trimmedLine.find(';', valueStart);
        if (valueEnd != std::string::npos) {
                std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
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
            std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
            if (value == "on") {
                location.file_upload = true;
                debuglog(GREEN, "Location file_upload: on");
            }
        }
    } 
    else if (trimmedLine.find("upload_dir") == 0) 
    {
        size_t valueStart = trimmedLine.find_first_not_of(" \t", 10);
        size_t valueEnd = trimmedLine.find(';', valueStart);

      if (valueEnd != std::string::npos) {
        std::string value = trimmedLine.substr(valueStart, valueEnd - valueStart);
        if (value[value.length() - 1] != '/') {
            value += '/';
        }
        location.upload_dir = value;
        debuglog(GREEN, "Location upload_dir: %s", value.c_str());
        }
    } 
    else if (trimmedLine.find("acceptedMethods") == 0) {
        size_t methodsStart = trimmedLine.find_first_not_of(" \t", 15);
        size_t openBrace = trimmedLine.find("{");
        size_t semiColon = trimmedLine.find(";");

        if (methodsStart != std::string::npos && openBrace != std::string::npos) {
            std::string methods = trimmedLine.substr(methodsStart, openBrace - methodsStart);
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
        else if (methodsStart != std::string::npos && semiColon != std::string::npos) {
                std::string methods = trimmedLine.substr(methodsStart, semiColon - methodsStart);
                std::istringstream methodStream(methods);
                std::string method;

            while (methodStream >> method) {
                location.acceptedMethods.push_back(method);
                debuglog(GREEN, "Location accepted method: %s", method.c_str());
            }
        }
    }
  }
}

void parseCgiBlock(const std::string &cgiContent, CGIData &cgiConfig) {
    std::istringstream iss(cgiContent);
    std::string line;

    while (std::getline(iss, line)) {
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos)
            continue; // Skip empty lines
        size_t end = line.find_last_not_of(" \t\n\r");
        std::string trimmedLine = line.substr(start, end - start + 1);

        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            continue; // Skip comments
        }

        if (trimmedLine.find("cgi_path_alias") == 0) {
            size_t valueStart = trimmedLine.find_first_not_of(" \t", 14);
            size_t valueEnd = trimmedLine.find(';', valueStart);

        std::string value;
        if (valueEnd != std::string::npos) {
            value = trimmedLine.substr(valueStart, valueEnd - valueStart);
        } 
        else {
            value = trimmedLine.substr(valueStart);
        }

        std::istringstream pathStream(value);
        std::string path, alias;

        pathStream >> path >> alias;
        if (!alias.empty() && alias[0] == '"' && alias[alias.size() - 1] == '"') {
            alias = alias.substr(1, alias.size() - 2);
        }

        cgiConfig.cgi_path_alias = std::make_pair(path, alias);
        debuglog(GREEN, "CGI path alias: %s -> %s", path.c_str(), alias.c_str());
    } 
    else if (trimmedLine.find("upload_dir") == 0) {
        size_t valueStart = trimmedLine.find_first_not_of(" \t", 10);
        size_t valueEnd = trimmedLine.find(';', valueStart);

        std::string value;
        if (valueEnd != std::string::npos) {
            value = trimmedLine.substr(valueStart, valueEnd - valueStart);
        } 
        else {
            value = trimmedLine.substr(valueStart);
        }

        cgiConfig.upload_dir = value;
        debuglog(GREEN, "CGI upload_dir: %s", value.c_str());
    } 
    else if (trimmedLine.find("file_extension") == 0) {
        size_t valueStart = trimmedLine.find_first_not_of(" \t", 14);
        size_t valueEnd = trimmedLine.find(';', valueStart);

        std::string value;
        if (valueEnd != std::string::npos) {
            value = trimmedLine.substr(valueStart, valueEnd - valueStart);
        } 
        else {
            value = trimmedLine.substr(valueStart);
        }

        std::istringstream extStream(value);
        std::string ext;

        while (extStream >> ext) {
            cgiConfig.cgi_extensions.push_back(ext);
            debuglog(GREEN, "CGI file extension: %s", ext.c_str());
        }
    }
    else if (trimmedLine.find("acceptedMethods") == 0) {
        size_t methodsStart = trimmedLine.find_first_not_of(" \t", 15); 
        size_t openBrace = trimmedLine.find("{");

        if (methodsStart != std::string::npos) {
            std::string methodsStr;
            if (openBrace != std::string::npos) {
                methodsStr = trimmedLine.substr(methodsStart, openBrace - methodsStart);
            } 
            else {
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

            while (methodStream >> method) {
                if (method != "{" && method != "}") {
                    cgiConfig.acceptedMethods.push_back(method);
                    debuglog(GREEN, "CGI acceptedMethods method: %s", method.c_str());
                }
            }
        }
    }
  }
}


template <typename T>
bool parseNumericValue(const std::string &line, const std::string &param, size_t paramLen, T &outValue) 
{
    size_t valueStart = line.find_first_not_of(" \t", paramLen);
    size_t valueEnd = line.find(';', valueStart);

    if (valueEnd != std::string::npos) {
        std::string valueStr = line.substr(valueStart, valueEnd - valueStart);
        outValue = static_cast<T>(atoi(valueStr.c_str()));
        return true;
    }
    return false;
}

size_t findClosingBrace(const std::string &content, size_t start) 
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



void debugprintConfigs(std::vector<ServerData>& servers, std::map<uint16_t, ServerData*> port_map_)
{
    debuglog(BLUE, "==== Configuration Summary ====");
    debuglog(BLUE, "Total servers: %zu", servers.size());

    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerData& server = servers[i];

        debuglog(BLUE, "\n----- Server %zu -----", i + 1);

        // Print ports
        if (!server.ports.empty()) {
            std::string portList;
            for (size_t j = 0; j < server.ports.size(); ++j) {
                if (j > 0) portList += ", ";
                char portStr[8];
                snprintf(portStr, sizeof(portStr), "%hu", server.ports[j]);
                portList += portStr;
            }
            debuglog(BLUE, "Ports: %s", portList.c_str());
        } else {
            debuglog(BLUE, "Ports: None defined");
        }

        // Print server names
        if (!server.server_names.empty()) {
            std::string nameList;
            for (size_t j = 0; j < server.server_names.size(); ++j) {
                if (j > 0) nameList += ", ";
                nameList += server.server_names[j];
            }
            debuglog(BLUE, "Server Names: %s", nameList.c_str());
        } else {
            debuglog(BLUE, "Server Names: None defined");
        }

        // Print basic server settings
        debuglog(BLUE, "Root: %s", server.root.c_str());
        debuglog(BLUE, "Index: %s", server.index.c_str());
        debuglog(BLUE, "Max Body Size: %lu bytes", server.maxBodySize);
        debuglog(BLUE, "Autoindex: %s",
        server.autoindex ? "on" : "off"); 
        debuglog(BLUE, "File Server: %s", server.file_server ? "on" : "off"); 
        debuglog(BLUE, "Upload Directory: %s", server.upload_dir.c_str());

        // Print error pages
        if (!server.error_pages.empty()) {
            debuglog(BLUE, "Error Pages:");
            std::map<int, std::string>::const_iterator it;
            for (it = server.error_pages.begin(); it !=
            server.error_pages.end(); ++it) {
                debuglog(BLUE, "  %d: %s", it->first, it->second.c_str());
            }
        }

        // Print location blocks
        if (server.has_locations && !server.location_blocks.empty()) {
            debuglog(BLUE, "Location Blocks (%zu):",
            server.location_blocks.size()); std::map<std::string,
            Location>::const_iterator it; for (it =
            server.location_blocks.begin(); it !=
            server.location_blocks.end(); ++it) {
                const std::string& path = it->first;
                const Location& loc = it->second;

                debuglog(BLUE, "  Location: %s", path.c_str());

                // Only print non-default values for clarity
                debuglog(BLUE, "    Autoindex: %s", loc.autoindex ? "on" :
                "off"); debuglog(BLUE, "    File Upload: %s", loc.file_upload
                ? "on" : "off"); debuglog(BLUE, "    Upload Dir: %s",
                loc.upload_dir.c_str());

                debuglog(BLUE, "    root : %s", loc.root.c_str());

                if (loc.internal) debuglog(BLUE, "    Internal: %d",
                loc.internal); else
                    debuglog(BLUE, "    Internal is not set\n ");
                     if (loc.return_directive.first != 0) {

                        debuglog(BLUE, "    Return: %d %s",
                        loc.return_directive.first,
                        loc.return_directive.second.c_str());
                    }

                if (!loc.acceptedMethods.empty()) {
                    std::string methods;
                    for (size_t j = 0; j < loc.acceptedMethods.size(); ++j) {
                        if (j > 0) methods += " ";
                        methods += loc.acceptedMethods[j];
                    }
                    debuglog(BLUE, "    Accepted Methods: %s",
                    methods.c_str());
                }
                //if(!loc.index.empty()) {
                   // debuglog(BLUE, "    Index: %s", loc.index.c_str());
               // }
                    debuglog(BLUE, " PRINTING ERROR PAGES");
                    std::map<int, std::string>::const_iterator it;
                    for (it = loc.error_pages.begin(); it !=
                    loc.error_pages.end(); ++it) {
                        debuglog(BLUE, "    Error Page: %d %s", it->first,
                        it->second.c_str());
                    }
            }
        }

        // Print CGI settings
        if (server.cgi_exists) {
            debuglog(BLUE, "CGI Settings:");
            debuglog(BLUE, "  Path Alias: %s -> %s",
                  server.cgiData.cgi_path_alias.first.c_str(),
                  server.cgiData.cgi_path_alias.second.c_str());
            debuglog(BLUE, "  Upload Dir: %s",
            server.cgiData.upload_dir.c_str());

            if (!server.cgiData.cgi_extensions.empty()) {
                std::string exts;
                for (size_t j = 0; j < server.cgiData.cgi_extensions.size();
                ++j) {
                    if (j > 0) exts += " ";
                    exts += server.cgiData.cgi_extensions[j];
                }
                debuglog(BLUE, "  File Extensions: %s", exts.c_str());
            }

            if (!server.cgiData.acceptedMethods.empty()) {
                std::string methods;
                for (size_t j = 0; j < server.cgiData.acceptedMethods.size();
                ++j) {
                    if (j > 0) methods += " ";
                    methods += server.cgiData.acceptedMethods[j];
                }
                debuglog(BLUE, "  Limited HTTP Methods: %s",
                methods.c_str());

            }

        }
    }

     debuglog(BLUE, "\nPort Mapping:");
     std::map<uint16_t, ServerData*>::const_iterator it;
     for (it = port_map_.begin(); it != port_map_.end(); ++it) {
         size_t serverIndex = it->second - &servers[0];  
         // Calculate server index 
         debuglog(BLUE, "  Port %hu: Server %zu", it->first,
         serverIndex);
        //PRINT THE WHOLE SERVER BLOCK
        debuglog(BLUE, "  Server Block: %d", servers[serverIndex].ports[0]);
        debuglog(BLUE, "  Server Block: %d", servers[serverIndex].ports[1]);
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].server_names[0].c_str());
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].server_names[1].c_str());
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].serverListenAddress.c_str());
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].root.c_str());
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].index.c_str());
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].upload_dir.c_str());
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].autoindex ? "on" : "off");
        debuglog(BLUE, "  Server Block: %s", servers[serverIndex].file_server ? "on" : "off");

    }


    debuglog(BLUE, "\n==== End of Configuration Summary ====");
}


}

