#pragma once

#include <map>
#include <string>
#include <ctime>

namespace Constants {
extern std::map<int, std::string> statusMessages;
extern std::map<std::string, std::string> mimeTypes;
extern const char *default_config_file;
void initStatusMessageMap();
void initMimeTypes();

extern int maxConnections;
extern int requestTimeout;
extern int responseTimeout;
extern int keepalive_timeout;
extern bool autoReload;
extern time_t cgi_child_timeout;
extern time_t client_timeout;

} // namespace Constants
