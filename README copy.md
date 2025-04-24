[![42](https://img.shields.io/badge/-Berlin-blue.svg?logo=data:image/svg%2bxml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4NCjwhLS0gR2VuZXJhdG9yOiBBZG9iZSBJbGx1c3RyYXRvciAxOC4xLjAsIFNWRyBFeHBvcnQgUGx1Zy1JbiAuIFNWRyBWZXJzaW9uOiA2LjAwIEJ1aWxkIDApICAtLT4NCjxzdmcgdmVyc2lvbj0iMS4xIg0KCSBpZD0iQ2FscXVlXzEiIHNvZGlwb2RpOmRvY25hbWU9IjQyX2xvZ28uc3ZnIiBpbmtzY2FwZTp2ZXJzaW9uPSIwLjQ4LjIgcjk4MTkiIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyIgeG1sbnM6c3ZnPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIgeG1sbnM6c29kaXBvZGk9Imh0dHA6Ly9zb2RpcG9kaS5zb3VyY2Vmb3JnZS5uZXQvRFREL3NvZGlwb2RpLTAuZHRkIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOmNjPSJodHRwOi8vY3JlYXRpdmVjb21tb25zLm9yZy9ucyMiIHhtbG5zOmlua3NjYXBlPSJodHRwOi8vd3d3Lmlua3NjYXBlLm9yZy9uYW1lc3BhY2VzL2lua3NjYXBlIg0KCSB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHhtbG5zOnhsaW5rPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5L3hsaW5rIiB4PSIwcHgiIHk9IjBweCIgdmlld0JveD0iMCAtMjAwIDk2MCA5NjAiDQoJIGVuYWJsZS1iYWNrZ3JvdW5kPSJuZXcgMCAtMjAwIDk2MCA5NjAiIHhtbDpzcGFjZT0icHJlc2VydmUiPg0KPHBvbHlnb24gaWQ9InBvbHlnb241IiBwb2ludHM9IjMyLDQxMi42IDM2Mi4xLDQxMi42IDM2Mi4xLDU3OCA1MjYuOCw1NzggNTI2LjgsMjc5LjEgMTk3LjMsMjc5LjEgNTI2LjgsLTUxLjEgMzYyLjEsLTUxLjEgDQoJMzIsMjc5LjEgIi8+DQo8cG9seWdvbiBpZD0icG9seWdvbjciIHBvaW50cz0iNTk3LjksMTE0LjIgNzYyLjcsLTUxLjEgNTk3LjksLTUxLjEgIi8+DQo8cG9seWdvbiBpZD0icG9seWdvbjkiIHBvaW50cz0iNzYyLjcsMTE0LjIgNTk3LjksMjc5LjEgNTk3LjksNDQzLjkgNzYyLjcsNDQzLjkgNzYyLjcsMjc5LjEgOTI4LDExNC4yIDkyOCwtNTEuMSA3NjIuNywtNTEuMSAiLz4NCjxwb2x5Z29uIGlkPSJwb2x5Z29uMTEiIHBvaW50cz0iOTI4LDI3OS4xIDc2Mi43LDQ0My45IDkyOCw0NDMuOSAiLz4NCjwvc3ZnPg0K)](https://42berlin.de) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) ![Version](https://img.shields.io/badge/version-1.0.0-blue) [![My Workflow](https://github.com/multitudes/42-Webserv/actions/workflows/ci.yml/badge.svg)](https://github.com/multitudes/42-Webserv/actions/workflows/ci.yml) 
 
# 42-Webserv

> "What I cannot create, I do not understand".
~ Richard Feynmann 

You can see the documentation page here: [here](https://multitudes.github.io/42-webserv/).  

[It is still work in progress! Config parsing not yet merged. It uses a hardcoded configuration for now]

## Goals
- Design a simple server that can handle multiple connections using multi-threading or asynchronous I/O. 
- Implement a basic request parser and response generator.
- Handle different HTTP methods (GET, POST, etc.). 
- Serve static files.
- Implement routing to handle different endpoints.
- CGI support.
- Implement configuration parsing.

Usage:  
```bash
./webserv [configuration file]
```

## Allowed functions
See the allowed_functions.md file here: [allowed_functions.md](docs/allowed_functions.md)

## Important
> You must never do a read or a write operation without going through poll() or select() first.

# The HTTP protocol and UNIX sockets
HTTP (Hypertext Transfer Protocol) was invented by Tim Berners-Lee at CERN (the European Organization for Nuclear Research) in 1989. The first version of HTTP, HTTP/0.9, was a simple protocol for transferring raw data across the internet. The more widely recognized version, HTTP/1.0, was specified in 1996, followed by HTTP/1.1 in 1997, which introduced persistent connections and other improvements. Today we have HTTP/2 and HTTP/3, but our server will handle HTTP/1.1.


# RFC's
RFC stands for "Request for Comments." It is a type of publication from the engineering and standards organizations for the internet, such as the Internet Engineering Task Force (IETF) and the Internet Society (ISOC). RFCs are used to describe methods, behaviors, research, or innovations applicable to the working of the internet and internet-connected systems.

Key Points about RFCs:
- Standardization: RFCs are often used to propose and standardize protocols, procedures, and policies.
- Numbering: Each RFC is assigned a unique number once it is published.
- Public Review: RFCs are open for public review and comments, which is part of the standardization process.
- Historical Importance: Many foundational internet protocols, such as HTTP, TCP/IP, and SMTP, were first described in RFCs.

Example:  
HTTP/1.1: Described in RFC 2616.  
TCP/IP: Described in RFC 793 and RFC 791.  

You can read a summary of the rfc2616 here [rfc2616-summary.md](docs/rfc2616-summary.md)

# CGI
CGI stands for Common Gateway Interface. It is a standard protocol used to enable web servers to execute external programs, typically scripts, and generate dynamic content for web pages. CGI scripts can be written in various programming languages, including Perl, Python, and C/C++.

Key Points about CGI:
- Dynamic Content: CGI allows web servers to generate dynamic content based on user input or other data.
- Language Agnostic: CGI scripts can be written in any programming language that can read from standard input and write to standard output.
- Execution: When a web server receives a request for a CGI script, it executes the script and sends the output back to the client as an HTTP response.
- Environment Variables: CGI scripts receive information about the request through environment variables, such as QUERY_STRING, REQUEST_METHOD, and CONTENT_TYPE.

For example:  
A user submits a form on a web page.  
The web server receives the form data and passes it to a CGI script.  
The CGI script processes the data (e.g., querying a database).  
The script generates an HTML response based on the processed data.  
The web server sends the HTML response back to the user's browser.  

Here is a simple example of a CGI script written in python that outputs "Hello, World!":
```C
#!/usr/bin/python

print("Content-Type: text/html")
print("content-length: 48")
print()
print("<html><body>")
print("<h1>Hello, World!</h1>")
print("</body></html>")
```

To use this script, you would place it in the CGI directory of your web server (often cgi-bin), and configure the server to execute it when accessed via a specific URL.
Interestingly the script above has 3 details that often are overlooked. 
1 - The first line is a shebang line that tells the operating system which interpreter to use to run the script. So the script is an executable.  
2 - The Content-Type header is set to text/html, which tells the browser how to interpret the response. But more importantly, the first header is missing, the HTTP version and the status code. The server will add it automatically. But how does the server know which status code to add? The server will add a 200 OK status code if the script exits with a 0 status code. If the script exits with a non-zero status code, the server will add a 500 Internal Server Error status code.  
3 - The script must output a blank line after the headers to indicate the end of the headers and the beginning of the response body.  

If the shebang path is not correct, the server will return a 500 error.  If the content length is not present the server might try to chunk the response (better) or return Constants::BUFFER_SIZE and truncate it (less ideal but depends of the project requirements).  

# Configuration files
A server configuration file is a file used to define the settings and parameters for a server's operation. These files are essential for customizing the behavior of the server, specifying how it handles requests, manages resources, and interacts with other systems. Configuration files are typically written in a plain text format and can be edited using any text editor.

Key Elements of a Server Configuration File:
- Server Directives: Instructions that control the server's behavior, such as listening ports, document root, and server name.
- Security Settings: Parameters for authentication, authorization, and encryption (e.g., SSL/TLS settings).
- Resource Limits: Limits on resources like memory usage, connection limits, and timeouts.
- Logging: Settings for logging server activity, including log file locations and log levels.
- Modules and Extensions: Configuration for enabling or disabling server modules and extensions.
- Virtual Hosts: Definitions for handling multiple domains or websites on a single server.

## Examples of Server Configuration Files:

Apache HTTP Server (httpd.conf or apache2.conf):    

```apache
ServerName www.example.com
DocumentRoot "/var/www/html"
<Directory "/var/www/html">
    Options Indexes FollowSymLinks
    AllowOverride None
    Require all granted
</Directory>
ErrorLog "/var/log/apache2/error.log"
CustomLog "/var/log/apache2/access.log" combined
```

NGINX (nginx.conf):   

```nginx
server {
    listen 80;
    server_name example.com;
    root /var/www/html;

    location / {
        try_files $uri $uri/ =404;
    }

    error_log /var/log/nginx/error.log;
    access_log /var/log/nginx/access.log;
}
```

caddy file:  

```caddy
example.com:2020 {
	root * /var/www/html
	file_server browse
	redir /old /new {
		# Redirect /old to /new with a 301 status code
	}
	route /show/* {
		# Handle cgi scripts
	}
}
```

For our project we decided to implement a syntax similar to the NGINX configuration file.


# Some Definitions
- **Socket**: A socket is an endpoint for communication between two machines over a network. It can be used to send and receive data, establish connections, and perform other network-related tasks. Sockets are identified by an IP address and a port number and are in the OSI layer 4 (Transport Layer) of the network stack.
- **Port**: A port is a communication endpoint in an operating system that allows multiple processes to use the same network interface. Ports are identified by numbers ranging from 0 to 65535 and are used to direct network traffic to specific applications or services running on a machine and are in the OSI layer 4 (Transport Layer) of the network stack.
- **TCP/IP** TCP/IP (Transmission Control Protocol/Internet Protocol) is a suite of communication protocols used to connect devices over the internet. It provides reliable, end-to-end communication between devices by breaking data into packets and routing them across networks. TCP/IP includes protocols like TCP, IP, UDP, and ICMP.
more about the TCP handshake here: [tcp-handshake.md](docs/tcp-handshake.md)

More definitions are here: [definitions.md](docs/definitions.md)
# Our Web Server
It is a team project. We are a team of 3 students and we split the work in 3 parts.
- The first part is the parsing of the configuration file and the server setup. More about it here: [config.md](docs/config.md)
- The second part is the handling of the incoming requests and the response generation.
- The third part is handling the sockets and the CGI support.

# Testing 
see the testing.md file here: [testing.md](docs/testing.md) and also how we use siege here [siege.md](docs/siege.md)

## links
Here is a good blog site that explains sockets and network programming in C:  
https://www.codequoi.com/en/sockets-and-network-programming-in-c/  

The Hypertext Transfer Protocol (HTTP) is an
application-level protocol for distributed, collaborative, hypermedia information systems. HTTP has been in use by the World-Wide Web global information initiative since 1990. Here is the HttP1.1 standard:   
https://datatracker.ietf.org/doc/html/rfc2616  

A classic, "Beej's Guide to Network Programming" by Brian "Beej" Hall:   
https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf

Sometimes useful, the comprehensive reference for C++ standard library.   
https://en.cppreference.com/w/  

Open Source Projects for inspiration:  
The source code of existing open-source web servers like NGINX and Apache HTTP Server.  
https://github.com/nginx/nginx  

Caddy, a modern web server with automatic HTTPS.  
https://caddyserver.com/  

tcp/ip rfc  
https://datatracker.ietf.org/doc/html/rfc791

zeroSSL,  the easiest way to issue free SSL certificates.    
https://zerossl.com/   

what are we building?
"Linus did it in 5 days, see where you can git" 
https://youtube.com/shorts/_lZV76JO3WU?si=GyMXzBFrhj3ws_oX

mozilla developer network  
https://developer.mozilla.org/en-US/docs/Web/HTTP/Overview  

blog post :  
https://blog.codinghorror.com/dont-reinvent-the-wheel-unless-you-plan-on-learning-more-about-wheels/  

RFC 793 about TCP:  
https://datatracker.ietf.org/doc/html/rfc793  

This readme is also available in the docs folder which is rendered as a GitHub Pages static site.  
It is failry easy to set up and can be used to document the project.  
Here is a small walkthrouh on how to set it up:  
https://github.com/nicolas-van/easy-markdown-to-github-pages  

files and sockets:
https://youtu.be/il4N6KjVQ-s?si=g6yGCTs1_IRZu9jm  

a nice 404 page  
https://training-lms.redhat.com/st_toolkit/common/pages/error404.html  

check this for doxigen graphs  
https://gist.github.com/CarloCattano/1f1db247c4eb8477a365e29eaf12aaf1  

CGI on nginx!
https://stackoverflow.com/questions/11667489/how-to-run-cgi-scripts-on-nginx
https://www.server-world.info/en/note?os=Ubuntu_20.04&p=nginx&f=6  

some git tips:  
https://sethrobertson.github.io/GitFixUm/fixup.html  
https://dangitgit.com/  


CGI  
https://www.grm.cuhk.edu.hk/~htlee/perlcourse/fileupload/fileupload2.html  


Webserv Testers
-Intra Tester  
-https://github.com/t0mm4rx/webserv/tree/main/tests  
-https://github.com/fredrikalindh/webserv_tester  
-https://github.com/hygoni/webserv_tester  

HTTP/1.1 vs HTTP/2 vs HTTP/3  
https://youtu.be/UMwQjFzTQXw?si=9p1x_e8wvDKlvo4L  
