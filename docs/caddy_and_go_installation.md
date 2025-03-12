# install caddy

You need to have go installed on your system

Here is a good start to install go on your system
https://go.dev/doc/install

basically you download from there the binary and then you extract it to /usr/local or better to sgoinfre since you dont have root access on your school computer.  
You also need to add the go binary to your PATH. 
In you bashrc ans zshrc add the following lines:  
```bash
export GOROOT=/home/lbrusa/sgoinfre/go
export PATH=$PATH:$GOROOT/bin
export GOPATH=/home/user/go/workspaceexport PATH=$PATH:
```
and source it.

verify that go is installed by running
```bash
go version
```

You need to go to the caddy website and download the source code if you dont have the root access. Navigate to the directory where you want to install caddy and run the following commands:  

```bash
$ git clone "https://github.com/caddyserver/caddy.git"
$ cd caddy/cmd/caddy/
$ go build
```

I put the caddy binary in my ~/.local/bin directory and added it to my PATH in my bashrc and zshrc.

```bash
export PATH=$PATH:$HOME/.local/bin
```


## Inside the caddyfile

Caddy is a powerful and flexible web server that uses a configuration file called the Caddyfile to define its behavior. The Caddyfile is a simple, human-readable text file that specifies how Caddy should handle incoming requests. Internally, Caddy parses the Caddyfile and converts it into a JSON structure that it uses to configure the server.

## Caddyfile

The Caddyfile is the configuration file for caddy. It is a text file that you can create in the root of your project. Here is an example of a Caddyfile:  

```caddyfile
# the first blocks are global settings, optional
{
	debug
	http_port 2020 # default if not specified
	auto_https off 
}
# Here is the server conf block
http://localhost:2020 {
	root * /home/lbrusa/Documents/caddyHTTP
	
	file_server

	encode zstd gzip

	redir /old-path /new-path

	route /new-path {
        respond "This is the new path"
    }

	route /another-path {
        respond "This is another path"
    }

    route /favicon.ico {
        respond 204  # No content
    }

	handle_errors {
	@404 {
		expression {http.error.status_code} == 404
	}
	respond @404 "Custom 404 Page" 404
    }
}
```

## errors
The syntax `respond @404 "Custom 404 Page" 404` in the Caddyfile is used to define a custom response for 404 errors. Here's a breakdown of the syntax:

- `respond`: This directive is used to send a simple response to the client.
- `@404`: This is a named matcher that matches requests resulting in a 404 error.
- `"Custom 404 Page"`: This is the response body that will be sent to the client.
- `404`: This is the HTTP status code for the response.

The `handle_errors` block is used to define custom error handling. The `@404` matcher is defined to catch 404 errors, and the `respond` directive is used to send a custom response for those errors.

Here is the full context of the `handle_errors` block:

```caddyfile
handle_errors {
	@404 {
		expression {http.error.status_code} == 404
	}
	respond @404 "Custom 404 Page" 404
}
```

In this example:
- The `handle_errors` block is used to handle errors.
- The `@404` matcher is defined to match requests that result in a 404 error.
- The `respond @404 "Custom 404 Page" 404` directive sends a custom response with the body "Custom 404 Page" and the status code 404.


## Caddy Structure

Caddy is built with a modular architecture, allowing users to extend its functionality with various modules.

### Example JSON Configuration

Here is an example of what the JSON configuration might look like for the above Caddyfile:

```json
{
    "apps": {
        "http": {
            "servers": {
                "srv0": {
                    "listen": [":2020"],
                    "routes": [
                        {
                            "handle": [
                                {
                                    "handler": "file_server",
                                    "root": "/home/lbrusa/Documents/caddyHTTP"
                                }
                            ]
                        },
                        {
                            "handle": [
                                {
                                    "handler": "encode",
                                    "encodings": {
                                        "gzip": {},
                                        "zstd": {}
                                    }
                                }
                            ]
                        },
                        {
                            "handle": [
                                {
                                    "handler": "static_response",
                                    "body": "This is the new path"
                                }
                            ],
                            "match": [
                                {
                                    "path": ["/new-path"]
                                }
                            ]
                        },
                        {
                            "handle": [
                                {
                                    "handler": "static_response",
                                    "body": "This is another path"
                                }
                            ],
                            "match": [
                                {
                                    "path": ["/another-path"]
                                }
                            ]
                        },
                        {
                            "handle": [
                                {
                                    "handler": "static_response",
                                    "body": "This is the new path",
                                    "status_code": 301
                                }
                            ],
                            "match": [
                                {
                                    "path": ["/old-path"]
                                }
                            ]
                        }
                    ]
                }
            }
        }
    }
}
```

### Modules in Caddy

Modules in Caddy are components that provide specific functionality, such as handling HTTP requests, serving files, encoding responses, and more. Each module is identified by a unique name and can be configured with specific parameters.

### Classes for JSON Representation

To represent this JSON configuration in a structured way, you would typically define classes or data structures that map to the JSON keys and values. Here is an example in Python using data classes:

```python
from dataclasses import dataclass, field
from typing import List, Dict, Any

@dataclass
class StaticResponse:
    handler: str
    body: str = ""
    status_code: int = 200

@dataclass
class Encode:
    handler: str
    encodings: Dict[str, Any]

@dataclass
class FileServer:
    handler: str
    root: str

@dataclass
class Directive:
    handle: List[Any]
    match: List[Dict[str, List[str]]] = field(default_factory=list)

@dataclass
class Server:
    listen: List[str]
    routes: List[Directive]

@dataclass
class HTTPApp:
    servers: Dict[str, Server]

@dataclass
class Apps:
    http: HTTPApp

@dataclass
class CaddyConfig:
    apps: Apps

# Example usage
config = CaddyConfig(
    apps=Apps(
        http=HTTPApp(
            servers={
                "srv0": Server(
                    listen=[":2020"],
                    routes=[
                        Directive(handle=[FileServer(handler="file_server", root="/home/lbrusa/Documents/caddyHTTP")]),
                        Directive(handle=[Encode(handler="encode", encodings={"gzip": {}, "zstd": {}})]),
                        Directive(handle=[StaticResponse(handler="static_response", body="This is the new path")], match=[{"path": ["/new-path"]}]),
                        Directive(handle=[StaticResponse(handler="static_response", body="This is another path")], match=[{"path": ["/another-path"]}]),
                        Directive(handle=[StaticResponse(handler="static_response", body="This is the new path", status_code=301)], match=[{"path": ["/old-path"]}])
                    ]
                )
            }
        )
    )
)
```

## adding cgi support
cgi in caddy is not default and you need to add it as a plugin. This is an example:

```bash
	# CGI configuration for hello.py
route /cgi-bin/hello.py {
	cgi {
		exec /usr/bin/python3 /home/lbrusa/Documents/caddyHTTP/cgi-bin/hello.py
	}
}

# CGI configuration for add.py
route /cgi-bin/add.py {
	cgi {
		exec /usr/bin/python3 /home/lbrusa/Documents/caddyHTTP/cgi-bin/add.py
	}
}
```

with multiple scripts
```
{
 {
"handle": [
	{
		"handler": "cgi",
		"exec": "/usr/bin/python3",
		"args": ["/home/lbrusa/Documents/caddyHTTP/cgi-bin/hello.py"]
	}
],
"match": [
	{
		"path": ["/cgi-bin/hello.py"]
	}
]
},
{
"handle": [
	{
		"handler": "cgi",
		"exec": "/usr/bin/python3",
		"args": ["/home/lbrusa/Documents/caddyHTTP/cgi-bin/add.py"]
	}
],
"match": [
	{
		"path": ["/cgi-bin/add.py"]
	}
]
}
}
```

## cgi with caddy

download xcaddy:
```bash
go install github.com/caddyserver/xcaddy/cmd/xcaddy@latest
```

If you need a plugin for cgi support you can use xcaddy.  
```bash
xcaddy build --with github.com/aksdb/caddy-cgi/v2
```

here is the cgi repo 
https://github.com/aksdb/caddy-cgi

## what is a reverse proxy
The `reverse_proxy` directive in the Caddyfile can be used to forward requests to an existing backend service running on (ex) `127.0.0.1:9005`.

Here is what you need to do for this to work:

Your Caddyfile should include the `reverse_proxy` directive to forward requests to the backend service. Here is an example configuration:

```caddyfile
{
	http_port 2020
}

http://localhost:2020 {
	encode zstd gzip

	# Reverse proxy for the API
	reverse_proxy /api/* 127.0.0.1:9005

	# Serve static files
	root * /home/lbrusa/Documents/caddyHTTP
	file_server browse
}
```

Run Caddy with the configured Caddyfile:

```sh
caddy run
```


### Example Backend Service

If you don't already have a backend service running on port 9005, you can create a simple one using Python's built-in HTTP server for demonstration purposes:

```sh
# Create a simple backend service using Python
mkdir -p /home/lbrusa/Documents/api
cd /home/lbrusa/Documents/api
echo "Hello from the API" > index.html

# Start the backend service on port 9005
python3 -m http.server 9005
```

With this setup, when you navigate to `http://localhost:2020/api/` in your browser, Caddy will forward the request to the backend service running on `127.0.0.1:9005`, and you should see the response "Hello from the API".

This configuration allows Caddy to act as a reverse proxy, forwarding specific requests to the backend service while serving static files from the specified root directory.