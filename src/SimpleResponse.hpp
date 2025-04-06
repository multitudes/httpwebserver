#pragma once

#include "HTTPConnxData.hpp"


enum CONTENT_TYPE {
    TEXT_PLAIN = 0,
    TEXT_HTML = 1
};

using std::string;

namespace SimpleResponse {

    void createResponse(HTTPConnxData &connections, string contentType, std::string response, int statusCode);

    void prepareFileResponse(HTTPConnxData &conn, long fileSize);

    void htmlErrorResponse(HTTPConnxData &connections, int statusCode);  

    void generatedHTMLResponse(HTTPConnxData &connection, int statusCode);

    void simpleStatusResponse(HTTPConnxData &connections, int statusCode);

} // namespace DirectoryListing