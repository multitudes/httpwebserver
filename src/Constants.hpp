#pragma once

#include <map>
#include <string>
#include <ctime>

namespace Constants {

extern std::map<int, std::string> statusMessages;
extern std::map<std::string, std::string> mimeTypes;
extern const char *default_config_file;
extern size_t BUFFER_SIZE;
extern int maxConnections;
extern int requestTimeout;
extern int responseTimeout;
extern int keepalive_timeout;
extern bool autoReload;
extern time_t cgi_child_timeout;
extern time_t client_timeout;

void initStatusMessageMap();
void initMimeTypes();

} // namespace Constants
