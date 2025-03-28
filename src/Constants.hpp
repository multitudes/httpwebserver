#pragma once

#include <map>
#include <string>


namespace Constants {
	extern std::map<int, std::string> statusMessages;
    extern std::map<std::string, std::string> mimeTypes;

	void initStatusMessageMap();
	void initMimeTypes();

} // namespace Constants