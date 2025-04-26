#include "Parser.hpp"
#include "debug.h"

std::string trimLine(const std::string &line) {
    size_t start = line.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
      return "";
      
    size_t end = line.find_last_not_of(" \t\n\r");
    return line.substr(start, end - start + 1);
  }

  long getCurrentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long)(tv.tv_sec) * 1000 + (long)(tv.tv_usec) / 1000;
  }



void debugprintConfigs(std::vector<ServerData> &servers,
    std::map<uint16_t, ServerData *> port_map_) {
debuglog(BLUE, "==== Configuration Summary ====");
debuglog(BLUE, "Total servers: %zu", servers.size());

for (size_t i = 0; i < servers.size(); ++i) {
const ServerData &server = servers[i];

debuglog(BLUE, "\n----- Server %zu -----", i + 1);

// Print ports
if (!server.ports.empty()) {
std::string portList;
for (size_t j = 0; j < server.ports.size(); ++j) {
if (j > 0)
portList += ", ";
char portStr[8];
snprintf(portStr, sizeof(portStr), "%hu", server.ports[j]);
portList += portStr;
}
debuglog(BLUE, "Ports: %s", portList.c_str());
} else {
debuglog(BLUE, "Ports: None defined");
}

// Print server names
if (!server.server_names.empty()) {
std::string nameList;
for (size_t j = 0; j < server.server_names.size(); ++j) {
if (j > 0)
nameList += ", ";
nameList += server.server_names[j];
}
debuglog(BLUE, "Server Names: %s", nameList.c_str());
} else {
debuglog(BLUE, "Server Names: None defined");
}

// Print basic server settings
debuglog(BLUE, "Root: %s", server.root.c_str());
debuglog(BLUE, "Index: %s", server.index.c_str());
debuglog(BLUE, "Max Body Size: %lu bytes", server.maxBodySize);
debuglog(BLUE, "Autoindex: %s", server.autoindex ? "on" : "off");
debuglog(BLUE, "File Server: %s", server.file_server ? "on" : "off");
debuglog(BLUE, "Upload Directory: %s", server.upload_dir.c_str());

// Print error pages
if (!server.error_pages.empty()) {
debuglog(BLUE, "Error Pages:");
std::map<int, std::string>::const_iterator it;
for (it = server.error_pages.begin(); it != server.error_pages.end();
++it) {
debuglog(BLUE, "  %d: %s", it->first, it->second.c_str());
}
}

// Print location blocks
if (server.has_locations && !server.location_blocks.empty()) {
debuglog(BLUE, "Location Blocks (%zu):", server.location_blocks.size());
std::map<std::string, Location>::const_iterator it;
for (it = server.location_blocks.begin();
it != server.location_blocks.end(); ++it) {
const std::string &path = it->first;
const Location &loc = it->second;

debuglog(BLUE, "  Location: %s", path.c_str());

// Only print non-default values for clarity
debuglog(BLUE, "    Autoindex: %s", loc.autoindex ? "on" : "off");
debuglog(BLUE, "    File Upload: %s", loc.file_upload ? "on" : "off");
debuglog(BLUE, "    Upload Dir: %s", loc.upload_dir.c_str());

debuglog(BLUE, "    root : %s", loc.root.c_str());

if (loc.internal)
debuglog(BLUE, "    Internal: %d", loc.internal);
else
debuglog(BLUE, "    Internal is not set\n ");
if (loc.return_directive.first != 0) {

debuglog(BLUE, "    Return: %d %s", loc.return_directive.first,
loc.return_directive.second.c_str());
}

if (!loc.acceptedMethods.empty()) {
std::string methods;
for (size_t j = 0; j < loc.acceptedMethods.size(); ++j) {
if (j > 0)
methods += " ";
methods += loc.acceptedMethods[j];
}
debuglog(BLUE, "    Accepted Methods: %s", methods.c_str());
}
// if(!loc.index.empty()) {
//  debuglog(BLUE, "    Index: %s", loc.index.c_str());
// }
debuglog(BLUE, " PRINTING ERROR PAGES");
std::map<int, std::string>::const_iterator it;
for (it = loc.error_pages.begin(); it != loc.error_pages.end(); ++it) {
debuglog(BLUE, "    Error Page: %d %s", it->first,
it->second.c_str());
}

}
}

// Print CGI settings
if (server.cgi_exists) {
debuglog(BLUE, "CGI Settings:");
debuglog(BLUE, "  Path Alias: %s -> %s",
server.cgiData.cgi_path_alias.first.c_str(),
server.cgiData.cgi_path_alias.second.c_str());
debuglog(BLUE, "  Upload Dir: %s", server.cgiData.upload_dir.c_str());

if (!server.cgiData.cgi_extensions.empty()) {
std::string exts;
for (size_t j = 0; j < server.cgiData.cgi_extensions.size(); ++j) {
if (j > 0)
exts += " ";
exts += server.cgiData.cgi_extensions[j];
}
debuglog(BLUE, "  File Extensions: %s", exts.c_str());
}

if (!server.cgiData.acceptedMethods.empty()) {
std::string methods;
for (size_t j = 0; j < server.cgiData.acceptedMethods.size(); ++j) {
if (j > 0)
methods += " ";
methods += server.cgiData.acceptedMethods[j];
}
debuglog(BLUE, "  Limited HTTP Methods: %s", methods.c_str());
}
}
}

debuglog(BLUE, "\nPort Mapping:");
std::map<uint16_t, ServerData *>::const_iterator it;
for (it = port_map_.begin(); it != port_map_.end(); ++it) {
size_t serverIndex = static_cast<size_t>(it->second - &servers[0]);
// Calculate server index
debuglog(BLUE, "  Port %hu: Server %zu", it->first, serverIndex);
}

debuglog(BLUE, "\n==== End of Configuration Summary ====");
}