#pragma once

#include "HTTPConnxData.hpp"
#include <map>


/**
 * This module is called when an incoming request is received
 * and header are parsed and it will validate the target url
 * agains the server configuration and change the state of
 * the connection to either CONN_FILE_REQUEST or CONN_CGI
 * or CONN_UPLOAD or CONN_SIMPLE_RESPONSE depending on the request
 * and the server configuration
 */
namespace URLMatcher {

void validateRequest(HTTPConnxData &conn);

} // namespace URLMatcher