# Configuration file for the webserver

This file is used to configure the webserver. It is used to define the server's behavior and settings. 

For our project we wanted to use a configuration file that we knew already existed and was used by many people. We chose to use the Nginx configuration file. However the Nginx doesnt support the cgi scripts. So we did a hybrid of Nginx and Apache configuration files.  

See here for an overview of different configuration files: [config_files.md](docs/config_files.md).  
At first we thought that Caddy would be a good candidate for a simple config file, but there is a lot under the hood, that we would not implement in our project. You can see here for an overview of the Caddy configuration file: [caddy_config.md](docs/caddy_configuration_docs.md).  

So in the end we decided that nginx would be our best bet. We used the nginx configuration file as a base and added the cgi script support from the apache configuration file.

An example of our configuration file with all cases covered would be:
```nginx
http {
    keepalive_timeout 5;
    
    server {
        listen 4244;
        listen 4245;
        server_name myWebserver someWebserver;
        root www;
        client_max_body_size 100000000;
        
        # Error pages
        error_page {
            400 www/error_pages/400.html
            403 www/error_pages/403.html
            404 www/error_pages/404.html
            405 www/error_pages/405.html
            418 www/error_pages/418.html
            500 www/error_pages/500.html
            502 www/error_pages/502.html
        }

        # Location blocks
        location /42 {
            return 301 http://42berlin.de/;
        }

        location /43 {
            autoindex on;
        }

        location /go {
            return 301 /here/index.html;
        }

        location /uploads {
            file_upload on;
        }

        # CGI Configuration
        cgi {
            cgi_path_alias /cgi-bin "/cgi-bin"
            cgi_extension .pl /bin/perl
            cgi_extension .py /bin/python3
            upload_dir www/uploads
        }

        # Accepted Methods
        servers GET POST DELETE PUT {
            deny all;
        }
    }

    # Second server
    server {
        listen 4246;
        server_name myWebserver someWebserver;
        root ./www/html;
    }

    # Third server
    server {
        listen 4247;
        server_name myWebserver someWebserver;
		root ./www/html;
    }

}
```

This would populate a base struct with the following values:
```cpp
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
};
```
and a struct dor each directive block:
```cpp
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
};
```
The cgi has its own block:
```cpp
struct CGIData {
  std::pair<std::string, std::string> cgi_path_alias; // Pair to store CGI path alias
  std::map<std::string, std::string> cgi_extensions; // Map of file extensions to CGI interpreters paths
  std::string upload_dir;
};
```

since the cgi uses scripts, I will have on the top of each script a shebang with the path to the interpreter. Therefore it is not necessary to store the interpreter in the configuration file.