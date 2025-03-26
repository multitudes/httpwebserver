# pragma once

#include "HTTPConnxData.hpp"
#include "ConfigData.hpp"
#include "debug.h"

namespace CGI {

// Start a CGI process for a connection
int prepareCGI(HTTPConnxDate& connx);

} // namespace CGI