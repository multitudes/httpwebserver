#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <stdint.h>

/* All the data structs being used in the config class */

/**
 * @brief CGI Configuration struct
 *
 * This struct will hold the CGI configuration data
 * from the configuration file. This will be used to
 * configure the CGI handler in the server.
 * ex of CGI Configuration - this is not nginx compliant because nginx doesnt support cgi
    cgi_path_alias /cgi-bin/ "www/secret_script_location/" {
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
  std::pair<std::string, std::string> cgi_path_alias; // Pair to store CGI path alias
  std::map<std::string, std::string> cgi_extensions; // Map of file extensions to CGI interpreters paths
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
  std::vector<std::string> acceptedMethods; // List of accepted HTTP methods GET, POST, PUT, DELETE, etc.
//   std::string redirect_to;
  bool autoindex; // will present a list of files in a directory
  bool file_upload; // will allow file uploads
  std::string upload_dir; // Directory to store uploaded files
  std::string alias; // for rerouting
  bool internal; // for internal only pages
  std::pair<int, std::string> return_directive; // Pair to store return directive
  std::map<int, std::string> error_pages; // Map of error codes to custom error pages location level 

  Directive() :
		acceptedMethods(),
		// redirect_to(""),
		autoindex(false),
		file_upload(false),
		upload_dir("/www/uploads"), // default upload dir
		alias(""),
		internal(false),
		return_directive(),
		error_pages()
		{}
};

/**
 * @brief Main configuration struct
 *
 * This is about setting the default for the ConfigDatas server blocks
 */
struct BaseConf {
  std::size_t maxConnections;
  int requestTimeout;   // read timeout - this is used!
  int responseTimeout;  // write timeout - this is used!

  int keepalive_timeout;      // The maximum time to keep an idle connection open.
  size_t maxBodySize;      // defaults to 0 for infinite if not specified
  std::map<std::string, std::string> headers;
  bool autoindex; // will present a list of files in a directory
  bool file_server; // it will serve everything it is default!
  std::vector<std::string> acceptedMethods;
  std::map<int, std::string> error_pages; // Map of error codes to custom error pages 
  std::string upload_dir; // Directory to store uploaded files

  CGIData cgiData;	

  BaseConf()
    : maxConnections(100),
      requestTimeout(60),
      responseTimeout(60),
      keepalive_timeout(30),
      maxBodySize(10000000),  // Changed from 0 to 1000 as per example TODO : amend
      headers(),
      autoindex(false),
      file_server(true),
      acceptedMethods(),
      error_pages(),
      upload_dir("./www/uploads") // TODO check if we do it this way

  {
    // Add default headers
    headers["Content-Type"] = "text/html";
    headers["Server"] = "nginx/1.18.0";
    headers["Connection"] = "keep-alive";
  }
};

struct ErrorPage {
    std::map<int, std::string> statusPages;
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
struct ConfigData: public BaseConf {
  std::string serverListenAddress;
  std::vector<uint16_t> ports; // one server block can have more than one port
  std::vector<std::string> server_names; 
  std::string index;
  std::string root; // can be relative or absolute path?
  std::vector<std::string> acceptedMethods; // At server level is a default which can be overridden at location
  std::map<std::string, Directive> location_blocks; // ex I write "location /42 { return 301 http://42berlin.de/; }  "/42" is the key maybe use location directive
  CGIData cgiData;
  // ErrorPage error_page;
  // std::vector<std::string> limit_except;

  // Constructor to initialize default values
  ConfigData()
	: BaseConf(), // Initialize BaseConf with default values
	serverListenAddress("localhost"),
	server_names(),
	index("index.html"),
	root("www"), // no forward slash preceding and no slash following 
	acceptedMethods(),
	location_blocks(),
	cgiData()
	// error_page(),
	// limit_except()
	{}
};