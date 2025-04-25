#ifndef URL_MATCHER_HPP
#define URL_MATCHER_HPP

#include "Config.hpp" // For Location type
#include <string>
#include <sys/stat.h>

// Forward declarations
struct HTTPConnxData;
namespace URLMatcher {
void validateRequest(HTTPConnxData &conn);
bool receiveAndParseRequest(HTTPConnxData &conn);
bool getConfigSetURLMatcherData(HTTPConnxData &conn);
void determineContentType(HTTPConnxData &conn, const std::string &path);
bool handleRegularFile(HTTPConnxData &conn, const std::string &path_for_stat,
                       const struct stat &path_stat);
bool handleIndexFile(HTTPConnxData &conn, const std::string &index_file_path,
                     const struct stat &index_stat);
bool handleDirectoryListing(HTTPConnxData &conn);
bool findCGIPathAlias(HTTPConnxData &conn);
void updateWithLocationBlockConfig(HTTPConnxData &conn);
bool handleChunkedData(HTTPConnxData &conn);
bool handleCookieUpdateRequest(HTTPConnxData &conn);
bool handleGETRequest(HTTPConnxData &conn);
bool handlePOSTRequest(HTTPConnxData &conn);
bool handleDELETERequest(HTTPConnxData &conn);
bool applyLocationBlockSettings(HTTPConnxData &conn, const Location &location);
void updatePathsFromLocation(HTTPConnxData &conn, const Location &location,
                             const std::string &locationPath);
} // namespace URLMatcher

#endif // URL_MATCHER_HPP
