# Caddy Configuration files

We looked at the most common configuration files for an HTTP server, including Apache, Nginx, Lighttpd and Caddy. Since this is a school program for a small http server we do not need the complexity of a full blown config file and we decided to settle for something simple. Caddy was the inspiration for our configuration file.

Settings can include details such as the server's port number, the root directory for serving files, the maximum number of connections, and other server-specific options.

To create a configuration file for your HTTP-only web server with CGI support in Caddy, you need to include various settings such as the port, host, server names, default error pages, client body size limits, and route configurations. Below is an example of how your Caddy configuration file might look:

### Caddy Configuration File example

```caddyfile
# Define the first server block
example.com {
    # Listen on port 80
    bind 0.0.0.0
    port 80

    # Set server names
    server_name example.com www.example.com

    # Default error pages
    handle_errors {
        @404 {
            expression {http.error.status_code} == 404
        }
        rewrite @404 /404.html
        file_server
    }

    # Limit client body size
    request_body {
        max_size 10MB
    }

    # Setup routes
    route / {
        root * /var/www/html
        file_server browse
    }

    route /images/* {
        root * /var/www/html/images
        file_server browse
    }

    route /upload {
        method POST
        respond "Upload endpoint"
    }

    route /redirect {
        redir http://example.com/
    }

    route /cgi-bin/* {
        root * /var/www/cgi-bin
        cgi {
            exec /usr/bin/python3
        }
    }
}

# Define the second server block
another.example.com {
    bind 0.0.0.0
    port 8080

    # Set server names
    server_name another.example.com

    # Default error pages
    handle_errors {
        @404 {
            expression {http.error.status_code} == 404
        }
        rewrite @404 /404.html
        file_server
    }

    # Limit client body size
    request_body {
        max_size 10MB
    }

    # Setup routes
    route / {
        root * /var/www/another
        file_server browse
    }

    route /images/* {
        root * /var/www/another/images
        file_server browse
    }

    route /upload {
        method POST
        respond "Upload endpoint"
    }

    route /redirect {
        redir http://another.example.com/
    }

    route /cgi-bin/* {
        root * /var/www/another/cgi-bin
        cgi {
            exec /usr/bin/python3
        }
    }
}
```

### Explanation

- **Global Options Block**: The `admin off` directive disables the Caddy admin API.
- **Server Blocks**: Each server block defines a virtual host with its own configuration.
- **Port and Host**: The `bind` and `port` directives specify the IP address and port to listen on.
- **Server Names**: The `server_name` directive sets the server names.
- **Error Pages**: The `handle_errors` block defines custom error pages.
- **Client Body Size**: The `request_body` block limits the size of the client body.
- **Directives**: The `route` blocks define various routes and their configurations.
  - **Root Directive**: Serves files from the specified root directory.
  - **Images Directive**: Serves files from the images directory.
  - **Upload Directive**: Restricts the route to POST method.
  - **Redirect Directive**: Redirects to another URL.
  - **CGI Directive**: Configures CGI scripts to be executed by a specified interpreter (e.g., Python).

This configuration file sets up two virtual hosts (`example.com` and `another.example.com`) with various routes, error handling, client body size limits, and CGI support. Adjust the paths and settings as needed for your specific use case.

