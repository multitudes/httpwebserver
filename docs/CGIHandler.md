# CGI

Common Gateway Interface (CGI) is a standard for external applications (like scripts in Perl, Python, PHP) to interact with web servers.  
When a web server receives a request for a CGI script, it spawns a new process to execute that script. The script receives input from the web server and generates output that the web server sends back to the client.  

**Request Methods:**
- **GET:** Data is sent in the URL as query parameters (e.g., `http://example.com/cgi-bin/add?num1=5&num2=3`).
- **POST:** Data is sent in the request body. This method is more secure for sensitive data.

* **Example CGI Script (Perl):**

```perl
#!/usr/bin/perl

use strict;
use warnings;

my $num1 = $ENV{'QUERY_STRING'};
my ($key, $value);
if ($num1 =~ /(\w+)=(\w+)/) {
    ($key, $value) = ($1, $2);
}

my $num2 = $ENV{'QUERY_STRING'};
if ($num2 =~ /(\w+)=(\w+)/) {
    ($key, $value) = ($1, $2);
}

my $sum = $num1 + $num2;

print "Content-Type: text/plain\n\n";
print "The sum of $num1 and $num2 is: $sum\n";
```

## rfc 3875 

RFC 3875 defines the Common Gateway Interface (CGI) Version 1.1.

In essence, it outlines how web servers should interact with external programs (like scripts in Perl, Python, etc.)

Here's a summary:

Purpose:

CGI provides a standard way for web servers to execute external programs and pass request data to them.
It is used to generate dynamic content based on user input or other data.
Environment Variables:

CGI scripts receive information from the web server through environment variables.
Important environment variables include:
REQUEST_METHOD: The HTTP method used (e.g., GET, POST).
QUERY_STRING: The query string part of the URL.
CONTENT_TYPE: The MIME type of the request body.
CONTENT_LENGTH: The length of the request body.
SCRIPT_NAME: The path to the CGI script.
REMOTE_ADDR: The IP address of the client.
SERVER_NAME: The server's hostname or IP address.
SERVER_PORT: The port number on which the request was received.
Input and Output:

CGI scripts read input from standard input (stdin) and environment variables.
They produce output by writing to standard output (stdout).
The output must include an HTTP header followed by a blank line and then the response body.
HTTP Headers:

CGI scripts must generate valid HTTP headers in their output.
Common headers include Content-Type and Status.
Security Considerations:

Validate and sanitize all input to prevent security vulnerabilities such as injection attacks.
Ensure proper permissions and isolation for CGI scripts to prevent unauthorized access.
Execution:

The web server executes the CGI script in a separate process.
The server passes request data to the script via environment variables and standard input.

## Environment variables
**Key Environment Variables in RFC 3875**

* **`SERVER_SOFTWARE`:** 
    - The name and version of the server software. 
    - Example: `Apache/2.4.52`

* **`SERVER_NAME`:** 
    - The server's host name, DNS alias, or IP address. 
    - Example: `www.example.com` 

* **`GATEWAY_INTERFACE`:** 
    - The version of the CGI specification being used. 
    - Example: `CGI/1.1`

* **`SERVER_PROTOCOL`:** 
    - The name and revision of the information protocol that this request conforms to. 
    - Example: `HTTP/1.1`

* **`SERVER_PORT`:** 
    - The port number on which the server is listening for requests. 
    - Example: `80` (for HTTP), `443` (for HTTPS)

* **`REQUEST_METHOD`:** 
    - The method used to make this request (e.g., `GET`, `POST`, `HEAD`, `PUT`).

* **`PATH_INFO`:** 
    - The part of the URL after the script name but before the query string. 
    - Example: For `/cgi-bin/script.pl/foo/bar`, `PATH_INFO` would be `/foo/bar`.

* **`PATH_TRANSLATED`:** 
    - The full server path to the script.

* **`SCRIPT_NAME`:** 
    - The virtual path to the script being executed.
    - Example: `/cgi-bin/script.pl`

* **`QUERY_STRING`:** 
    - The query string, if any, from the URL.
    - Example: `?name=John&age=30`

* **`CONTENT_TYPE`:** 
    - The MIME type of the data being sent in the request body (for POST requests).

* **`CONTENT_LENGTH`:** 
    - The length of the data being sent in the request body (for POST requests).

* **`REMOTE_ADDR`:** 
    - The IP address of the remote host (the client).

* **`REMOTE_HOST`:** 
    - The hostname of the remote host (if available).

* **`HTTP_*` Headers:** 
    - Any HTTP headers included in the request are passed as environment variables. 
    - For example: `HTTP_USER_AGENT`, `HTTP_ACCEPT`, `HTTP_COOKIE`.

These are some of the most important environment variables. RFC 3875 defines others, but these are generally the most commonly used.

**Note:** The specific environment variables and their values may vary slightly depending on the web server implementation.

By accessing these environment variables within your CGI scripts, you can obtain crucial information about the incoming request and tailor your script's behavior accordingly.


## References
RFC 3875: The Common Gateway Interface (CGI) Version 1.1  
https://www.ietf.org/rfc/rfc3875.txt

