# CONFIGURATION FILES

the below is from the copilot ai, not tested. just for an idea of the syntax of different web servers configuration files.

## Nginx Configuration example

```nginx
http {
    server {
        listen 80;
        server_name example.com;

        # Default error pages
        error_page 404 /404.html;
        error_page 500 502 503 504 /50x.html;

        # Limit client body size
        client_max_body_size 1M;

        location / {
            root /var/www/html;
            index index.html index.htm;
            autoindex off; # Turn off directory listing
        }

        location /images/ {
            root /var/www/html;
            autoindex on; # Turn on directory listing
        }

        location /redirect {
            return 301 http://example.com/;
        }

        location /upload {
            acceptedMethods POST {
                deny all;
            }
        }
    }

    server {
        listen 8080;
        server_name another.example.com;

        location / {
            root /var/www/another;
            index index.html index.htm;
        }
    }
}
```

## Caddy Configuration example

```caddyfile
example.com {
    # Choose the port and host
    bind 0.0.0.0
    port 80

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
        max_size 1MB
    }

    # Setup routes
    route / {
        root * /var/www/html
        file_server browse
    }

    route /images/* {
        root * /var/www/html
        file_server browse
    }

    route /redirect {
        redir http://example.com/
    }

    route /upload {
        method POST
        respond "Upload endpoint"
    }
}

another.example.com {
    bind 0.0.0.0
    port 8080

    route / {
        root * /var/www/another
        file_server
    }
}
```
### example caddy path with CGI
```bash
http://localhost:2019 {
    root * /home/lbrusa/Documents/caddyHTTP
    file_server

    @add path /add
    cgi @add /path/to/cgi-bin/add
}
```

## Apache Configuration example

```apache
<VirtualHost *:80>
    ServerName example.com

    # Default error pages
    ErrorDocument 404 /404.html
    ErrorDocument 500 /50x.html

    # Limit client body size
    LimitRequestBody 1048576

    # Setup routes
    DocumentRoot /var/www/html
    <Directory /var/www/html>
        Options -Indexes # Turn off directory listing
        AllowOverride None
        Require all granted
    </Directory>

    <Directory /var/www/html/images>
        Options +Indexes # Turn on directory listing
    </Directory>

    Redirect /redirect http://example.com/

    <Location /upload>
        <LimitExcept POST>
            Require all denied
        </
<Location /upload>
        <LimitExcept POST>
            Require all denied
        </LimitExcept>
    </Location>
</VirtualHost>

<VirtualHost *:8080>
    ServerName another.example.com

    DocumentRoot /var/www/another
    <Directory /var/www/another>
        Options -Indexes
        AllowOverride None
        Require all granted
    </Directory>
</VirtualHost>
```

- Nginx: Uses server blocks to define servers, location blocks for routes, and directives like client_max_body_size to limit client body size.
- Caddy: Uses a simple, human-readable configuration file with route blocks for routes and request_body for limiting client body size.
- Apache: Uses <VirtualHost> blocks to define servers, <Directory> and <Location> blocks for routes, and directives like LimitRequestBody to limit client body size
