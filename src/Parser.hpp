#pragma once

#include "Config.hpp"
#include "ServerData.hpp"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/time.h>
#include <vector>

struct ServerBlockInfo {
    std::string content;
    size_t startPos;
    size_t endPos;
  };
  
namespace Parser {

extern long starttime;

void parse(std::string filename, std::vector<ServerData> &servers,
           std::map<uint16_t, ServerData *> &port_map_);
           
std::string OpenReadConfigFile(std::string filename);
std::string abstratHttpContent(std::string content);
std::string extractGlobalConfig(const std::string &httpContent);

void parsePortToServer(std::vector<ServerData> &servers,
    std::map<uint16_t, ServerData *> &port_map_);

void parseGlobalSettings(const std::string &httpContent, BaseConf &baseConfig);
void parseMaxBodySize(std::string &trimmedLine, BaseConf &baseConfig);
void parseAutoIndex(std::string &trimmedLine, BaseConf &baseConfig);
int getAutoindexCode(const std::string &value);
std::string abstractErrorPageBlock(std::string &trimmedLine, const std::string &httpContent, BaseConf &baseConfig);
void parseErrorPageBlock(const std::string &blockContent, BaseConf &baseConfig);
std::string extractPathFromLine(const std::string &line, size_t spacePos);
size_t findClosingBrace(const std::string &content, size_t start);
void addServerIfValid(std::vector<ServerData> &servers, ServerData &serverData, int serverBlockCount);

void parseServerBlocks(const std::string &httpContent, std::vector<ServerData> &servers, BaseConf &baseConfig);
bool isOnlyWhitespace(const std::string &str);
bool isValidServerKeyword(const std::string &content, size_t pos);
bool findNextServerBlock(const std::string &httpContent, size_t pos, 
    ServerBlockInfo &blockInfo);
void parseServerBlock(const std::string &serverBlockContent, ServerData &serverData, std::set<int> &portset);
void parseServerListenAddress(const std::string &trimmedLine, ServerData &serverData);
void parseServerRoot(std::string trimmedLine, ServerData &serverData);
void parseServerPort(std::string trimmedLine, ServerData &serverData, std::set<int> &portset);
void parseServerName(std::string &trimmedLine, ServerData &serverData);
void parseServerIndax(std::string &trimmedLine, ServerData &serverData);
void parseAccceptedMethods(std::string &trimmedLine, ServerData &serverData);

void parseLocationBlocks(const std::string &serverBlockContent, const std::string &trimmedLine, ServerData &serverData);
std::string extractLocationPath(const std::string &trimmedLine);
std::string extractLocationContent(const std::string &serverBlockContent, const std::string &trimmedLine);
void parseLocationBlock(const std::string &locationContent, Location &location, ServerData &serverData);
void initBasicVariables(Location &location, ServerData &serverData);
void parseLocationAutoIndex(std::string &trimmedLine, Location &location);
void parseLocationReturn(std::string trimmedLine, Location &location);
void parseLocationRoot(std::string trimmedLine, Location &location);
void parseLocationFileUpload(std::string &trimmedLine, Location &location);
void parseLocationUploadDir(std::string trimmedLine, Location &location);
void parseLocationAccceptedMethods(std::string &trimmedLine, Location &location);

void parseCgiConfig(const std::string &trimmedLine, const std::string &serverBlockContent, ServerData &serverData);
std::string extractCgiBlockContent(const std::string &line, const std::string &serverContent);
void parseCgiBlock(const std::string &cgiContent, CGIData &cgiConfig);
void parseCgiPathAlias(std::string &trimmedLine, CGIData &cgiConfig);
void parseCgiUploadDir(std::string &trimmedLine, CGIData &cgiConfig);
void parseCgiFileExtension(std::string &trimmedLine, CGIData &cgiConfig);
void parseCGIAcceptedMethods(std::string &trimmedLine,CGIData &cgiConfig);

template <typename T>
bool parseNumericValue(const std::string &line, const std::string &param, size_t paramLen, T &outValue);

}

std::string trimLine(const std::string &line);
long getCurrentTimeMillis();
void debugprintConfigs(std::vector<ServerData> &servers,
    std::map<uint16_t, ServerData *> port_map_);