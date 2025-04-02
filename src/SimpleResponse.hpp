
#pragma once

#include "HTTPConnxData.hpp"


enum CONTENT_TYPE {
    TEXT_PLAIN = 0,
    TEXT_HTML = 1
};

using std::string;

namespace SimpleResponse {

    void addHTTPHeader(HTTPConnxData &connections, string contentType, std::string response, int statusCode);

    void htmlErrorResponse(HTTPConnxData &connections, int statusCode);  

    void simpleStatusResponse(HTTPConnxData &connections, int statusCode);

} // namespace DirectoryListing