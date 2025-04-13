#pragma once

#include "HTTPConnxData.hpp"
#include "ServerData.hpp"
#include "debug.h"

namespace CGI {

// Start a CGI process for a connection
int prepareCGI(HTTPConnxData &connx);
void setCGIEnv(HTTPConnxData &connx);
std::string ensureTrailinSlash(std::string path);
std::string removeLeadingSlash(std::string path);
} // namespace CGI
