#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

/* All the data structs being used in the config class */

/**
 * @brief CGI Configuration struct
 *
 * This struct will hold the CGI configuration data
 * from the configuration file. This will be used to
 * configure the CGI handler in the server.
 * ex of CGI Configuration - this is not nginx compliant because nginx doesnt
 support cgi cgi_path_alias /cgi-bin/ "www/secret_script_location/" {
        cgi_extensions .pl .py .cgi;
        cgi_interpreter .pl /usr/bin/perl;
    cgi_interpreter .cgi /usr/bin/python3;
        cgi_interpreter .py /usr/bin/python3;
        max_body_size 1000;
    upload_dir /var/www/uploads;
    }

 * ex I will write  cgi_path_alias /cgi-bin/ "www/secret_script_location/"
 */
struct CGIData {
  std::pair<std::string, std::string>
      cgi_path_alias; // Pair to store CGI path alias
  std::map<std::string, std::string>
      cgi_extensions; // Map of file extensions to CGI interpreters paths
  std::string upload_dir;

  CGIData() {}
};

/**
 * @brief Server configuration struct
 *
 * In the server configuration struct we will have an array of directives
 * ex of a directive:
 * location / {
 * 	root /var/www/html;
 */
struct Directive {
  std::vector<std::string> acceptedMethods; // List of accepted HTTP methods
                                            // GET, POST, PUT, DELETE, etc.
                                            //   std::string redirect_to;
  bool autoindex;         // will present a list of files in a directory
  bool file_upload;       // will allow file uploads
  std::string upload_dir; // Directory to store uploaded files
  std::string alias;      // for rerouting
  bool internal;          // for internal only pages
  std::pair<int, std::string>
      return_directive; // Pair to store return directive
  std::map<int, std::string>
      error_pages; // Map of error codes to custom error pages location level

  Directive()
      : acceptedMethods(),
        // redirect_to(""),
        autoindex(false), file_upload(false),
        upload_dir("/www/uploads"), // default upload dir
        alias(""), internal(false), return_directive(), error_pages() {}
};

/**
 * @brief Main configuration struct
 *
 * This is about setting the default for the ConfigDatas server blocks
 */
struct BaseConf {
  size_t maxBodySize; // will be parsed

  std::size_t maxConnections; // will be hardcoded
  int requestTimeout;  // read timeout - will be hardcoded
  int responseTimeout; // write timeout - will be hardcoded
  int keepalive_timeout; // The maximum time to keep an idle connection open. will be hardcoded
  std::map<std::string, std::string> defaultheaders;
  bool autoindex;   // will present a list of files in a directory
  bool file_server; // it will serve everything it is default!
  std::vector<std::string> acceptedMethods;
  std::map<int, std::string>
      error_pages;        // Map of error codes to custom error pages
  std::string upload_dir; // Directory to store uploaded files

  CGIData cgiData;

  BaseConf()
      : maxConnections(100), requestTimeout(60), responseTimeout(60),
        keepalive_timeout(30),
        maxBodySize(
            10000000), // Changed from 0 to 1000 as per example TODO : amend
        defaultheaders(), autoindex(false), file_server(true), acceptedMethods(),
        error_pages(),
        upload_dir("./www/uploads") // TODO check if we do it this way

  {
    // Add default defaultheaders
    defaultheaders["Content-Type"] = "text/html";
    defaultheaders["Server"] = "nginx/1.18.0";
    defaultheaders["Connection"] = "keep-alive";
  }
};

// If no error page is specified, the default error page will be used
struct ErrorPage {
  std::map<int, std::string> errorPages;
};

struct LocationDirective {
  bool autoindex;
  bool internal;
  std::string alias;
  std::pair<int, std::string> return_directive;
  ErrorPage error_page;
  std::vector<std::string> limit_except;

  LocationDirective() : autoindex(false), internal(false) {}
};

/**
 * @brief Server configuration struct
 *
 * provisional template for one server configuration struct
 * which will contain all the necessary data from our
 * conf file. we might have an array of these but port assignment
 * should be unique
 * can have more than one server name in the configuration.
 * This is typically used for virtual hosting,
 * where a single server can respond to requests for multiple domain names.
 */
struct ConfigData : public BaseConf {
  std::string serverListenAddress;
  std::vector<uint16_t> ports; // one server block can have more than one port
  std::vector<std::string> server_names;
  std::string index;
  std::string root; // can be relative or absolute path?
  std::vector<std::string>
      acceptedMethods; // At server level is a default which can be overridden
                       // at location
  std::map<std::string, Directive>
      location_blocks; // ex I write "location /42 { return 301
                       // http://42berlin.de/; }  "/42" is the key maybe use
                       // location directive
  CGIData cgiData;
  std::vector<std::string> limit_except; // allowed methods
  ErrorPage error_pages; // Map of error codes to custom error pages

  // Constructor to initialize default values
  ConfigData()
      : BaseConf(), // Initialize BaseConf with default values
        serverListenAddress("localhost"), server_names(), index("index.html"),
        root("www"), // no forward slash preceding and no slash following
        acceptedMethods(), location_blocks(), cgiData(), limit_except(),
		error_pages()
  {}
};


/**
 * List of status codes
    100 = "Continue";
    101 = "Switching Protocols";
    102 = "Processing";
    103 = "Early Hints";
    200 = "OK";
    201 = "Created";
    202 = "Accepted";
    203 = "Non-Authoritative Information";
    204 = "No Content";
    205 = "Reset Content";
    206 = "Partial Content";
    207 = "Multi-Status";
    208 = "Already Reported";
    226 = "IM Used";
    300 = "Multiple Choices";
    301 = "Moved Permanently";
    302 = "Found";
    303 = "See Other";
    304 = "Not Modified";
    305 = "Use Proxy";
    306 = "Switch Proxy";
    307 = "Temporary Redirect";
    308 = "Permanent Redirect";
    404 = "error on Wikimedia";
    400 = "Bad Request";
    401 = "Unauthorized";
    402 = "Payment Required";
    403 = "Forbidden";
    404 = "Not Found";
    405 = "Method Not Allowed";
    406 = "Not Acceptable";
    407 = "Proxy Authentication Required";
    408 = "Request Timeout";
    409 = "Conflict";
    410 = "Gone";
    411 = "Length Required";
    412 = "Precondition Failed";
    413 = "Payload Too Large";
    414 = "URI Too Long";
    415 = "Unsupported Media Type";
    416 = "Range Not Satisfiable";
    417 = "Expectation Failed";
    418 = "I'm a teapot";
    421 = "Misdirected Request";
    422 = "Unprocessable Content";
    423 = "Locked";
    424 = "Failed Dependency";
    425 = "Too Early";
    426 = "Upgrade Required";
    428 = "Precondition Required";
    429 = "Too Many Requests";
    431 = "Request Header Fields Too Large";
    451 = "Unavailable For Legal Reasons";
    500 = "Internal Server Error";
    501 = "Not Implemented";
    502 = "Bad Gateway";
    503 = "Service Unavailable";
    504 = "Gateway Timeout";
    505 = "HTTP Version Not Supported";
    506 = "Variant Also Negotiates";
    507 = "Insufficient Storage";
    508 = "Loop Detected";
    510 = "Not Extended";
    511 = "Network Authentication Required";
 */