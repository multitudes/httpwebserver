#pragma once

#include "HTTPConnxData.hpp"

using std::string;

namespace Responses {

void createResponse(HTTPConnxData &connections, string contentType,
                    std::string response, int statusCode);
void prepareFileResponse(HTTPConnxData &conn, long fileSize);
void htmlErrorResponse(HTTPConnxData &connections, int statusCode);
void generatedHTMLResponse(HTTPConnxData &connection, int statusCode);
void simpleStatusResponse(HTTPConnxData &connections, int statusCode);
bool serveCustomErrorPage(HTTPConnxData &conn, const string &errorPagePath,
                          int statusCode);

} // namespace Responses