# pragma once

#include "HTTPConnxData.hpp"
#include "ServerData.hpp"
#include "debug.h"

namespace CGI {

// Start a CGI process for a connection
int prepareCGI(HTTPConnxData& connx);

} // namespace CGI
