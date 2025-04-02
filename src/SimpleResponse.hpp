#pragma once

#include "HTTPConnxData.hpp"


enum CONTENT_TYPE {
    TEXT_PLAIN = 0,
    TEXT_HTML = 1
};

using std::string;

namespace SimpleResponse {

    void addHTTPHeader(HTTPConnxData &connections, string contentType, std::string response, int statusCode);

    // Functions for file responses
    void prepareFileResponse(HTTPConnxData &conn, string contentType, long fileSize);
    void prepareFileResponse(HTTPConnxData &conn, long fileSize); // Overloaded version that uses conn.content_type

    void htmlErrorResponse(HTTPConnxData &connections, int statusCode);  

    void simpleStatusResponse(HTTPConnxData &connections, int statusCode);

} // namespace DirectoryListing