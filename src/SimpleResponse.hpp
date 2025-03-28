
#pragma once

#include "HTTPConnxData.hpp"


enum CONTENT_TYPE {
    TEXT_PLAIN = 0,
    TEXT_HTML = 1
};


namespace SimpleResponse {

    void addHTTPHeader(HTTPConnxData &connections, int content_type, std::string response);

    void htmlErrorResponse(HTTPConnxData &connections, int errorCode);  

} // namespace DirectoryListing