[![42](https://img.shields.io/badge/-Berlin-blue.svg?logo=data:image/svg%2bxml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgiPz4NCjwhLS0gR2VuZXJhdG9yOiBBZG9iZSBJbGx1c3RyYXRvciAxOC4xLjAsIFNWRyBFeHBvcnQgUGx1Zy1JbiAuIFNWRyBWZXJzaW9uOiA2LjAwIEJ1aWxkIDApICAtLT4NCjxzdmcgdmVyc2lvbj0iMS4xIg0KCSBpZD0iQ2FscXVlXzEiIHNvZGlwb2RpOmRvY25hbWU9IjQyX2xvZ28uc3ZnIiBpbmtzY2FwZTp2ZXJzaW9uPSIwLjQ4LjIgcjk4MTkiIHhtbG5zOnJkZj0iaHR0cDovL3d3dy53My5vcmcvMTk5OS8wMi8yMi1yZGYtc3ludGF4LW5zIyIgeG1sbnM6c3ZnPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIgeG1sbnM6c29kaXBvZGk9Imh0dHA6Ly9zb2RpcG9kaS5zb3VyY2Vmb3JnZS5uZXQvRFREL3NvZGlwb2RpLTAuZHRkIiB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iIHhtbG5zOmNjPSJodHRwOi8vY3JlYXRpdmVjb21tb25zLm9yZy9ucyMiIHhtbG5zOmlua3NjYXBlPSJodHRwOi8vd3d3Lmlua3NjYXBlLm9yZy9uYW1lc3BhY2VzL2lua3NjYXBlIg0KCSB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHhtbG5zOnhsaW5rPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5L3hsaW5rIiB4PSIwcHgiIHk9IjBweCIgdmlld0JveD0iMCAtMjAwIDk2MCA5NjAiDQoJIGVuYWJsZS1iYWNrZ3JvdW5kPSJuZXcgMCAtMjAwIDk2MCA5NjAiIHhtbDpzcGFjZT0icHJlc2VydmUiPg0KPHBvbHlnb24gaWQ9InBvbHlnb241IiBwb2ludHM9IjMyLDQxMi42IDM2Mi4xLDQxMi42IDM2Mi4xLDU3OCA1MjYuOCw1NzggNTI2LjgsMjc5LjEgMTk3LjMsMjc5LjEgNTI2LjgsLTUxLjEgMzYyLjEsLTUxLjEgDQoJMzIsMjc5LjEgIi8+DQo8cG9seWdvbiBpZD0icG9seWdvbjciIHBvaW50cz0iNTk3LjksMTE0LjIgNzYyLjcsLTUxLjEgNTk3LjksLTUxLjEgIi8+DQo8cG9seWdvbiBpZD0icG9seWdvbjkiIHBvaW50cz0iNzYyLjcsMTE0LjIgNTk3LjksMjc5LjEgNTk3LjksNDQzLjkgNzYyLjcsNDQzLjkgNzYyLjcsMjc5LjEgOTI4LDExNC4yIDkyOCwtNTEuMSA3NjIuNywtNTEuMSAiLz4NCjxwb2x5Z29uIGlkPSJwb2x5Z29uMTEiIHBvaW50cz0iOTI4LDI3OS4xIDc2Mi43LDQ0My45IDkyOCw0NDMuOSAiLz4NCjwvc3ZnPg0K)](https://42berlin.de) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) ![Version](https://img.shields.io/badge/version-1.0.0-blue) 

> "What I cannot create, I do not understand."
~ Richard Feynman

# HTTP Web Server

A lightweight HTTP 1.1 web server implementation built in C++ for the 42 school curriculum. This server handles multiple connections via multiplexing (poll/select), serves static files, processes various HTTP methods, and supports CGI scripts.

## Team Project
Our team of 3 divided the project into key components:
- Configuration file parsing and server setup
- Request handling and response generation
- Socket management and CGI implementation
- Documentation, testing, and CI/CD pipeline

## Usage
```bash
./webserv [configuration file]
```

Our server implements both CGI support and file upload capabilities, following an nginx-like configuration syntax.
## Table of Contents
- [Where it all started - The HTTP protocol and UNIX sockets](#where-it-all-started---the-http-protocol-and-unix-sockets)
- [RFC's](#rfcs)
- [The Project](#the-project)
  - [Configuration files](#configuration-files)
- [Multiplexing](#multiplexing)
- [The tcp/ip sockets](#the-tcpip-sockets)
  - [Handling requests](#handling-requests)
- [CGI](#cgi)
- [Testing](#testing)
- [Some Definitions](#some-definitions)
- [Resources](#resources)
  - [HTTP Standards and References](#http-standards-and-references)
  - [Programming Guides](#programming-guides)
  - [Open Source Projects](#open-source-projects)
  - [CGI Resources](#cgi-resources)
  - [Testing Resources](#testing-resources)
  - [Miscellaneous](#miscellaneous)
  - [Git Tips](#git-tips)

# Where it all started - The HTTP protocol and UNIX sockets
HTTP (Hypertext Transfer Protocol) was invented by Tim Berners-Lee at CERN (the European Organization for Nuclear Research) in 1989. The first version of HTTP, HTTP/0.9, was a simple protocol for transferring raw data across the internet. The more widely recognized version, HTTP/1.0, was specified in 1996, followed by HTTP/1.1 in 1997, which introduced persistent connections and other improvements. Today we have HTTP/2 and HTTP/3, but our server will need to handle HTTP/1.1 only.

# RFC's
RFC stands for "Request for Comments."  
It is a type of publication from the engineering and standards organizations for the internet, such as the Internet Engineering Task Force (IETF) and the Internet Society (ISOC). RFCs are used to describe methods, behaviors, research, or innovations applicable to the working of the internet and internet-connected systems.

For this project, we will refer to the following RFCs:
- [rfc2616](rfc2616.pdf): Hypertext Transfer Protocol -- HTTP/1.1 
- [rfc793](rfc793.pdf): Transmission Control Protocol
- [rfc791](rfc791.pdf): Internet Protocol
- [rfc3875](rfc3875.pdf): The Common Gateway Interface (CGI) Version 1.1

Because not everything is still relevant for us in the rfc2616, we summarized the most important parts. You can read a summary of the rfc2616 here [rfc2616-summary.md](rfc2616-summary.md).  
And a summary of the rfc791 is here [rfc791-summary.md](rfc791-summary.md).
The rfc3875 is not very long and we used it as reference for the CGI part of the project.

# The Project
See the allowed_functions.md file here: [allowed_functions.md](allowed_functions.md)

## Configuration files
A server configuration file is a file used to define the settings and parameters for a server's operation. These files are essential for customizing the behavior of the server, specifying how it handles requests, manages resources, and interacts with other systems. Configuration files are typically written in a plain text format and can be edited using any text editor.

As per subject requirements in the configuration file, our server should be able to:
- Choose the port and host of each ’server’.
- Set up the server_names or not.
- The first server for a host:port will be the default for this host:port 
- Set up default error pages
- Set the maximum allowed size for client request bodies 
- Set up routes with one or multiple of the following rules/configurations: a list of accepted HTTP methods for the route,  an HTTP redirect,  a directory or file where the requested file should be located, enable or disable directory listing, and set up a CGI route.
a default file to serve when the request is for a directory. Allow the route to accept uploaded files and configure where they should be saved.  
◦ Define a list of accepted HTTP methods for the route.
◦ Define an HTTP redirect.
◦ Define a directory or file where the requested file should be located 
- We can take inspiration from the ’server’ section of the NGINX configuration file  

See [here](config.md) for more about configuration files.

For our project we decided to implement a syntax similar to the NGINX configuration file.

# Multiplexing

This is one of the key topic of this project for 42. As per subject requirements:
> You must never do a read or a write operation without going through poll() or select() first.
The core of the project is to handle multiplexing. It allows multiple connections to share the same network resources efficiently. In the context of web servers, multiplexing is used to handle multiple client connections without simultaneously without blocking or slowing down the server. We are not allowed to use multithreading except for the CGI server. 

This is the main function simplified:
```Cpp
while (true) {
    int status = poll(&pollfds[0], (nfds_t)pollfds.size(), POLLTIMEOUT);
	if (status == -1) {
		[...]
	} else if (status == 0) {
		[... timeout ...]; continue;
	} else {
		[ ... loop on the fds and handle the events ]
	}
}
```
where pollfds is a vector of pollfd structures. The poll() function will wait for events on multiple file descriptors. When an event occurs, poll() will return, and we can check which file descriptor triggered the event. We can then read or write data on that file descriptor as needed.
If there are no new connections the poll() function will block to save resources and return 0 after the timeout. If there are new connections the poll() function will return and we can handle the events.  

# The tcp/ip sockets
> Sockets are a method of IPC that allow data to be exchanged between applications, either on the same host (computer) or on different hosts connected by a network. The first widespread implementation of the sockets API appeared with 4.2BSD in 1983, and this API has been ported to virtually every UNIX implementation, as well as most other operating systems. - Kerrisk, Michael. The Linux Programming Interface (p. 1136). No Starch Press. Kindle Edition.

During this project I read a lot from this book which I recommend.

From the configuration we have a list of ports and the server will create a tcp/ip socket for each port. We will then bind the socket to the port and start listening for incoming connections. 

We will use the poll() function to monitor multiple file descriptors for events. The poll() function is part of the POSIX standard and is used to wait for events on multiple file descriptors. It is similar to the select() function.

The poll() function takes an array of pollfd structures, each of which represents a file descriptor to monitor and the events to watch for. 

The pollfd structure has the following members:
- fd: The file descriptor to monitor.
- events: The events to watch for (e.g., POLLIN for data to read).
- revents: The events that occurred on the file descriptor (set by poll() after the call).

We create an array of pollfd structures to hold the server sockets. We are going to loop on those server sockets and set the events we want to monitor to POLLIN. We then call poll() to wait for events on the server sockets. When an event occurs, we check which socket triggered the event and accepting the connection will generate a new file descriptor, which we then add to the pollfd array. The client requests will arrive on this new file descriptor.

## Handling requests
When a client sends a first request to the server, the server will receive the request on one of the server sockets, generate a new fd and add it to the pollfds. The server will then read the request from that file descriptor, parse it, and generate a response. The response will be sent back to the client using the same file descriptor.
The important thing to not here is that we are going to read in a non blocking way. We will read a buffer size of bytes and then go back to poll() to check if there is more data to read. This way we will not block and give each connection a fair share of the server resources.

- We check if we got the headers. The delimiter for the headers is "\r\n\r\n". If we did not get the headers we will go back to poll() and wait for more data. 
- Once we get the headers we check if it is a get request. in this case there is no body so we can pass the headers to the response generator.
- if it is a multipart request we will read until we have the body and pass the headers and the body to the response generator.
- if it is a chunked request, since we do not handle them we will return a 501 Not Implemented response.
- in the CGI case, we need to create a child process, pass the headers in the environment variables, and pass the body to to the stdin of the child process. There will be two pipes at the moment of the creation of the child, one for the stdin and one for the stdout. We will add the pipe stdout and the pipe stdin to the pollfds.  
Adding the pipe stdin to the fdpoll will allow us to return to poll and read from client and write to the child until end of body. After that the child might send us some data and we will write it to the client passing through poll again until the child process is done.

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

# Testing 
See the testing.md file here: [testing.md](testing.md) and also how we use siege here [siege.md](siege.md)

# Some Definitions
- **Socket**: A socket is an endpoint for communication between two machines over a network. It can be used to send and receive data, establish connections, and perform other network-related tasks. Sockets are identified by an IP address and a port number and are in the OSI layer 4 (Transport Layer) of the network stack.
- **Port**: A port is a communication endpoint in an operating system that allows multiple processes to use the same network interface. Ports are identified by numbers ranging from 0 to 65535 and are used to direct network traffic to specific applications or services running on a machine and are in the OSI layer 4 (Transport Layer) of the network stack.
- **TCP/IP** TCP/IP (Transmission Control Protocol/Internet Protocol) is a suite of communication protocols used to connect devices over the internet. It provides reliable, end-to-end communication between devices by breaking data into packets and routing them across networks. TCP/IP includes protocols like TCP, IP, UDP, and ICMP.
more about the TCP handshake here: [tcp-handshake.md](tcp-handshake.md)

More definitions are here: [definitions.md](definitions.md)

## Resources

### HTTP Standards and References
- [HTTP/1.1 Standard RFC2616](https://datatracker.ietf.org/doc/html/rfc2616) - The Hypertext Transfer Protocol (HTTP) specification used by the World Wide Web since 1990
- [TCP Protocol RFC793](https://datatracker.ietf.org/doc/html/rfc793) - Transmission Control Protocol specification
- [IP Protocol RFC791](https://datatracker.ietf.org/doc/html/rfc791) - Internet Protocol specification
- [Mozilla Developer Network - HTTP Overview](https://developer.mozilla.org/en-US/docs/Web/HTTP/Overview) - Comprehensive HTTP documentation

### Programming Guides
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf) - Classic guide by Brian "Beej" Hall
- [C++ Standard Library Reference](https://en.cppreference.com/w/) - Comprehensive reference for C++ standard library
- [Sockets and Network Programming in C](https://www.codequoi.com/en/sockets-and-network-programming-in-c/) - Blog tutorials on socket programming
- [Files and Sockets Tutorial](https://youtu.be/il4N6KjVQ-s?si=g6yGCTs1_IRZu9jm) - Video guide on handling files and sockets

### Open Source Projects
- [NGINX Web Server](https://github.com/nginx/nginx) - Source code of the popular NGINX web server
- [Caddy Web Server](https://caddyserver.com/) - A modern web server with automatic HTTPS

### CGI Resources
- [CGI on NGINX](https://stackoverflow.com/questions/11667489/how-to-run-cgi-scripts-on-nginx) - Stack Overflow guide
- [Server World CGI Setup Guide](https://www.server-world.info/en/note?os=Ubuntu_20.04&p=nginx&f=6) - Tutorial for CGI setup
- [File Upload with CGI](https://www.grm.cuhk.edu.hk/~htlee/perlcourse/fileupload/fileupload2.html) - Guide for handling file uploads with CGI

### Testing Resources
- [Intra Tester](https://github.com/t0mm4rx/webserv/tree/main/tests)
- [WebServ Tester by fredrikalindh](https://github.com/fredrikalindh/webserv_tester)
- [WebServ Tester by hygoni](https://github.com/hygoni/webserv_tester)

### Miscellaneous
- [ZeroSSL](https://zerossl.com/) - Free SSL certificate issuance
- [Don't Reinvent the Wheel](https://blog.codinghorror.com/dont-reinvent-the-wheel-unless-you-plan-on-learning-more-about-wheels/) - Blog post on code reuse
- [GitHub Pages with Markdown](https://github.com/nicolas-van/easy-markdown-to-github-pages) - Setup guide for GitHub Pages documentation
- [HTTP Protocol Comparison](https://youtu.be/UMwQjFzTQXw?si=9p1x_e8wvDKlvo4L) - HTTP/1.1 vs HTTP/2 vs HTTP/3
- [Linus on Git](https://youtube.com/shorts/_lZV76JO3WU?si=GyMXzBFrhj3ws_oX) - "Linus did it in 5 days, see where you can git"
- [404 Page Example](https://training-lms.redhat.com/st_toolkit/common/pages/error404.html) - Example of a well-designed 404 page
- [Doxygen Graph Examples](https://gist.github.com/CarloCattano/1f1db247c4eb8477a365e29eaf12aaf1) - Helpful for documentation

### Git Tips
- [Git Fixum Guide](https://sethrobertson.github.io/GitFixUm/fixup.html) - How to fix Git mistakes
- [Dang It, Git!](https://dangitgit.com/) - Solutions for common Git problems