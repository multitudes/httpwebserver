# bouncing

I read this in the requirements for the webserver... 
Your server must never block and the client can be bounced properly if necessary.

What would mean bounce clients properly?

In the context of a web server, "bouncing" a client typically means rejecting or disconnecting the client connection under certain conditions. This can be necessary to ensure that the server remains responsive and does not get overwhelmed by too many connections or by clients that are misbehaving.

### Reasons to Bounce Clients

1. **Too Many Connections**: If the server is handling too many connections and cannot accept new ones.
2. **Timeouts**: If a client takes too long to send a request or to read a response.
3. **Invalid Requests**: If the client sends malformed or invalid requests.
4. **Resource Limits**: If the server is running low on resources (e.g., memory, CPU).

### How to Bounce Clients

1. **Close the Connection**: Simply close the client socket.
2. **Send an Error Response**: Send an HTTP error response (e.g., 503 Service Unavailable) before closing the connection.
3. **Rate Limiting**: Implement rate limiting to control the number of requests a client can make in a given time period.

### Example: Bouncing Clients in Your Server

Here is an example of how to implement client bouncing:

```cpp
void handleClientConnx
(size_t i) {
    char buffer[BUFSIZ + 1];
    ssize_t bytes_read;
    int clientfd = pollfds[i].fd;
    bytes_read = recv(clientfd, buffer, BUFSIZ, 0);
    if (bytes_read <= 0) {
     [...]
    } else {
       [...]
        // Check for invalid requests or other conditions to bounce the client
        if (is_invalid_request(buffer)) {
            debug("[Server] Invalid request from client socket %d\n", clientfd);
            sendErrorResponse(clientfd, 400, "Bad Request");
            close(clientfd);
            deleteFromPollfds(i);
            return;
        }

      [...]
        std::string response;
        try {
            response = Resolver::handleRequest(buffer, conf);
        } catch (const std::exception& e) {
            debug("[Server] Error handling request: %s\n", e.what());
            response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }
        debug("Response: %s\n", response.c_str());

        sendDataToSocket(i, response);
    }
}

bool is_invalid_request(const char* request) {
    //  check for invalid requests
    return true;
}

void sendErrorResponse(int clientfd, int status_code, const char* status_message) {
    char response[BUFSIZ];
    snprintf(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n", status_code, status_message);
    send(clientfd, response, strlen(response), 0);
}
```


## max connections
```bash
ulimit -n   
1048576
```	

