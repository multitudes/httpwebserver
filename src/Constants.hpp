#pragma once

#include <map>
#include <string>


namespace Constants {
	extern std::map<int, std::string> statusMessages;
    extern std::map<std::string, std::string> mimeTypes;
	extern const char* default_config_file;
	void initStatusMessageMap();
	void initMimeTypes();

	extern int maxConnections;
    extern int requestTimeout;
    extern int responseTimeout;
    extern int keepalive_timeout;


} // namespace Constants