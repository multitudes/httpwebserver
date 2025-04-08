#ifndef URL_MATCHER_HPP
#define URL_MATCHER_HPP

#include <string>
#include <sys/stat.h>

// Forward declarations
class HTTPConnxData;

namespace URLMatcher
{
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

  void validateRequest(HTTPConnxData &conn);
}

#endif // URL_MATCHER_HPP
