#include "Constants.hpp"
#include "debug.h"
#include <string>

namespace Constants {

std::map<int, std::string> statusMessages;
std::map<std::string, std::string> mimeTypes;
const char *default_config_file = "config/default.conf";
size_t BUFFER_SIZE = 8192; // 8KB buffer size
int maxConnections = 200;
int requestTimeout = 10;
int responseTimeout = 10;
int keepalive_timeout = 15;
bool autoReload = false;
time_t cgi_child_timeout = 1; // this is in case of an endless loop - all our cgi are faster
time_t client_timeout = 15;

void initStatusMessageMap() {
  debuglog(YELLOW, "Initializing status code to status text mapping");
  statusMessages[100] = "Continue";
  statusMessages[101] = "Switching Protocols";
  statusMessages[102] = "Processing";
  statusMessages[103] = "Early Hints";
  statusMessages[200] = "OK";
  statusMessages[201] = "Created";
  statusMessages[202] = "Accepted";
  statusMessages[203] = "Non-Authoritative Information";
  statusMessages[204] = "No Content";
  statusMessages[205] = "Reset Content";
  statusMessages[206] = "Partial Content";
  statusMessages[207] = "Multi-Status";
  statusMessages[208] = "Already Reported";
  statusMessages[226] = "IM Used";
  statusMessages[300] = "Multiple Choices";
  statusMessages[301] = "Moved Permanently";
  statusMessages[302] = "Found";
  statusMessages[303] = "See Other";
  statusMessages[304] = "Not Modified";
  statusMessages[305] = "Use Proxy";
  statusMessages[306] = "Switch Proxy";
  statusMessages[307] = "Temporary Redirect";
  statusMessages[308] = "Permanent Redirect";
  statusMessages[400] = "Bad Request";
  statusMessages[401] = "Unauthorized";
  statusMessages[402] = "Payment Required";
  statusMessages[403] = "Forbidden";
  statusMessages[404] = "Not Found";
  statusMessages[405] = "Method Not Allowed";
  statusMessages[406] = "Not Acceptable";
  statusMessages[407] = "Proxy Authentication Required";
  statusMessages[408] = "Request Timeout";
  statusMessages[409] = "Conflict";
  statusMessages[410] = "Gone";
  statusMessages[411] = "Length Required";
  statusMessages[412] = "Precondition Failed";
  statusMessages[413] = "Payload Too Large";
  statusMessages[414] = "URI Too Long";
  statusMessages[415] = "Unsupported Media Type";
  statusMessages[416] = "Range Not Satisfiable";
  statusMessages[417] = "Expectation Failed";
  statusMessages[418] = "I'm a teapot";
  statusMessages[421] = "Misdirected Request";
  statusMessages[422] = "Unprocessable Entity";
  statusMessages[423] = "Locked";
  statusMessages[424] = "Failed Dependency";
  statusMessages[425] = "Too Early";
  statusMessages[426] = "Upgrade Required";
  statusMessages[428] = "Precondition Required";
  statusMessages[429] = "Too Many Requests";
  statusMessages[431] = "Request Header Fields Too Large";
  statusMessages[451] = "Unavailable For Legal Reasons";
  statusMessages[500] = "Internal Server Error";
  statusMessages[501] = "Not Implemented";
  statusMessages[502] = "Bad Gateway";
  statusMessages[503] = "Service Unavailable";
  statusMessages[504] = "Gateway Timeout";
  statusMessages[505] = "HTTP Version Not Supported";
  statusMessages[506] = "Variant Also Negotiates";
  statusMessages[507] = "Insufficient Storage";
  statusMessages[508] = "Loop Detected";
  statusMessages[510] = "Not Extended";
  statusMessages[511] = "Network Authentication Required";
}

void initMimeTypes() {
  debuglog(YELLOW, "Initializing extension to MIME type mapping");

  // Web
  mimeTypes[".html"] = "text/html";
  mimeTypes[".htm"] = "text/html";
  mimeTypes[".css"] = "text/css";
  mimeTypes[".js"] = "application/javascript";
  mimeTypes[".xml"] = "application/xml";
  mimeTypes[".json"] = "application/json";

  // Text
  mimeTypes[".txt"] = "text/plain";
  mimeTypes[".csv"] = "text/csv";
  mimeTypes[".md"] = "text/markdown";
  mimeTypes[".sh"] = "text/x-shellscript";

  // Images
  mimeTypes[".jpg"] = "image/jpeg";
  mimeTypes[".jpeg"] = "image/jpeg";
  mimeTypes[".png"] = "image/png";
  mimeTypes[".gif"] = "image/gif";
  mimeTypes[".bmp"] = "image/bmp";
  mimeTypes[".svg"] = "image/svg+xml";
  mimeTypes[".ico"] = "image/x-icon";
  mimeTypes[".webp"] = "image/webp";

  // Documents
  mimeTypes[".pdf"] = "application/pdf";
  mimeTypes[".doc"] = "application/msword";
  mimeTypes[".docx"] = "application/msword";
  mimeTypes[".xls"] = "application/vnd.ms-excel";
  mimeTypes[".xlsx"] = "application/vnd.ms-excel";
  mimeTypes[".zip"] = "application/zip";

  // Multimedia
  mimeTypes[".mp3"] = "audio/mpeg";
  mimeTypes[".mp4"] = "video/mp4";
  mimeTypes[".webm"] = "video/webm";
}

} // namespace Constants