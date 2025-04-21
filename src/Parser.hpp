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

namespace Parser {

extern long starttime;

void parse(std::string filename, std::vector<ServerData> &servers,
           std::map<uint16_t, ServerData *> &port_map_);
std::string OpenReadConfigFile(std::string filename);
std::string abstratHttpContent(std::string content);
std::string trimLine(const std::string &line);
void parsePortToServer(std::vector<ServerData> &servers,
    std::map<uint16_t, ServerData *> &port_map_);
std::string extractGlobalConfig(const std::string &httpContent);
void parseGlobalSettings(const std::string &httpContent, BaseConf &baseConfig);
void parseMaxBodySize(std::string &trimmedLine, BaseConf &baseConfig);
void parseAutoIndex(std::string &trimmedLine, BaseConf &baseConfig);
std::string abstractErrorPageBlock(std::string &trimmedLine, const std::string &httpContent, BaseConf &baseConfig);
void parseErrorPageBlock(const std::string &blockContent, BaseConf &baseConfig);
size_t findClosingBrace(const std::string &content, size_t start);
template <typename T>
bool parseNumericValue(const std::string &line, const std::string &param,
                       size_t paramLen, T &outValue);
void parseServerBlocks(const std::string &httpContent,
                       std::vector<ServerData> &servers, BaseConf &baseConfig);
void parseServerBlock(const std::string &serverBlockContent,
                      ServerData &ServerData, std::set<int> &Portset);
void parseLocationBlock(const std::string &locationContent, Location &location,
                        ServerData &ServerData);
void parseCgiBlock(const std::string &cgiContent, CGIData &cgiConfig);
long getCurrentTimeMillis();
void debugprintConfigs(std::vector<ServerData> &servers,
                       std::map<uint16_t, ServerData *> port_map_);

} // namespace Parser
