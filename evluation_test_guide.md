# Check the code and ask questions

## 1. Ask explanations about the basics of an HTTP server

```bash
An HTTP server is software (or a system) that understands and responds to HTTP requests from clients, typically web browsers or tools like curl. Its main job is to serve web content—like HTML pages, images, CSS files, or even data—in response to client requests.
```

## 2. Ask what function the group used for I/O Multiplexing
```bash
poll() is a system call in POSIX-compliant systems that lets a server monitor multiple file descriptors to see if I/O operations can be performed without blocking—perfect for handling many simultaneous client connections.
```

## 3. Ask if they use only one select() (or equivalent) and how they've managed the server to accept and the client to read/write.
```bash
One poll() Call: The server uses only one poll() call to handle multiple connections simultaneously. It’s efficient since it allows the server to wait for events on many sockets at once without blocking on any one socket.

Efficient Resource Management: By using poll(), the server doesn't need to block on each socket individually. Instead, it can handle multiple clients in a non-blocking way, making it scalable and able to serve many clients concurrently.
```



# Configuration

## 1. how to test:
 Setup multiple servers with different hostnames (use something like: `curl --resolve example.com:80:127.0.0.1 http://example.com/`)
## command in terminal: 
```bash
        curl --resolve myWebserver:4244:127.0.0.1 http://myWebserver:4244/
        curl --resolve myWebserver2:4246:127.0.0.1 http://myWebserver2:4246/
        curl --resolve myWebserver3:4247:127.0.0.1 http://myWebserver3:4247/
```
## 2. how to test:
        Setup default error page (try to change the error 404).

## run on brower
```bash
         http://localhost:4244/unknown
```

## 3. how to test
        Limit the client body (use: curl -X POST -H "Content-Type: plain/text" --data "BODY IS HERE write something shorter or longer than body limit").
## coomand in terminal 
        first set the maxbodysize is 10, then upload bodysize greater that 10
```bash
        curl -X POST -H "Content-Type: text/plain" --data "Hello, file upload test" http://localhost:4244/upload
```

        if the bodysize is greater than maxbodysize, it will return 413 error page
        otherwise will upload successfully

## 4. how to test:
        Setup a default file to search for if you ask for a directory.
## run in terminal:
```bash
        curl http://localhost:4244  
```
will show the content of the index.html

## 5.how to test
        Setup a list of methods accepted for a certain route (e.g., try to delete something with and without permission).

## run in terminal:
 first add a text.txt file in the images location, and make sure the configuration file of image block allow delete

 ```bash
         curl -X DELETE http://localhost:4244/images/test.txt
```
this command will delete the test.txt file


now  add a text.txt file in the images location, and change the configuration file of images does not allow delete, and restart the server(if autoreload is off)
run the command 
  ```bash
         curl -X DELETE http://localhost:4244/images/test.txt
```
test.txt will not be deleted, and return error page 405 


# Basic checks

## 1. GET Request

```bash
        curl -v http://localhost:4244/index.html
        
        
        telnet localhost 4244 (first this command and then run the next two line command)

        GET /index.html HTTP/1.1
        Host: localhost



