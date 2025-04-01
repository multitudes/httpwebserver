#pragma once

// #include <stdbool.h>
#include "HTTPConnxData.hpp"

namespace DirectoryListing {

    bool getDIRListing(HTTPConnxData &connections, std::string full_path);

} // namespace DirectoryListing