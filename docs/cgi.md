# CGI

## env variables

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


## testing




chunked and POST
```bash
echo -n -e "This is the first chunk.\nAnd this is the second chunk.\nFinally, the third chunk." | \
    curl -v -X POST \
         -H "Content-Type: text/plain" \
         -H "Transfer-Encoding: chunked" \
         --data-binary @- \
         http://localhost:4244/cgi/hello.py
```

DELETE
```bash
curl -v -X DELETE http://localhost:4244/cgi/hello.py/resource/to/delete
```