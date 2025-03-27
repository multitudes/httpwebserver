#include "DirectoryListing.hpp"
#include "HTTPConnxData.hpp"
#include "debug.h"
#include <fcntl.h>

namespace DirectoryListing {
    
    bool getDIRListing(HTTPConnxData &connection) {
        debuglog(RED, "Request content: %s", connection.data.request.c_str());
        debuglog(YELLOW, "Opening file directory.html for fd %d",
            connection.file_fd);
            
        connection.file_fd = open("directory.html", O_RDONLY);
        
        return true;
    }

} // namespace DirectoryListing