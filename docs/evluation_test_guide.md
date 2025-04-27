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
```  
```bash
        telnet localhost 4244 (first this command and then run the next two line command)

        GET / HTTP/1.1
        Host: localhost:4244 (with two times enter)
        the terminal will show the index.html file, then ctrl + ] to end this session , you will see a telnet prompt, type quit to end the telnet
        from terminal you will see below info
```
```bash
        telnet localhost 4244
        Trying 127.0.0.1...
        Connected to localhost.
        Escape character is '^]'.
        GET / HTTP/1.1
        Host: localhost:4244

        # After response shows:
        ^]
        telnet> quit
        Connection closed.
```

## 2. POST Request need to be solved

```bash
        echo "name=ChatGPT&msg=Hello" > post_data.txt (create a file)
        curl -v -X POST -d @post_data.txt http://localhost:4244/upload
```
```bash
        telnet localhost 4244
        Trying 127.0.0.1...
        Connected to localhost.
        Escape character is '^]'.
        GET / HTTP/1.1
        Host: localhost:4244

        # After response shows:
        ^]
        telnet> quit
        Connection closed.
```
the reponse should be 200 ok

## 3. DELETE Request
add a file called delete_this.txt in the images directory
```bash
        curl -v -X DELETE http://localhost:4244/images/delete_this.txt
```
it will delete file delete_this.txt with 200 ok response

add a file called delete-this.txt in the images directory
```bash
        telnet localhost 4244

        DELETE /images/delete_this.txt HTTP/1.1
        Host: localhost:4244 
```

## 4. UNKNOWN / Invalid Request
```bash
        curl -v -X FOO http://localhost:4244/
```
invalid request, return bad request

## 5. File download
```bash
        curl -v http://localhost:4244/upload/hello.txt -o hello.txt
```
it will save the file hello.txt from directory uploads in the current directory

# Check with a browser
```bash
        localhost:4244/42/ (redirection)
```
```bash
        http://localhost:4244/uploads/ (autoindex on to list directory)
```
```bash
        localhost:4244/unknown/ (will show a  404 not found error page)
```

# Siege & stress test

## siege test command
```bash
        siege -c180 -b -t1m http://localhost:4244/index.html
```
Explanation of flags:
    -c180: simulate 180 concurrent users
    -b: benchmark mode (no delay between requests)
    -t1m: run for 1 minute
    http://localhost:4244/index.html: the URL to test

##  Monitor memory usage
While running the siege, open another terminal and run:
```bash
        top -p $(pidof webserv)
```
or 
```bash
        ps -o pid,rss,cmd -p $(pidof webserv)
        watch -n 1 'ps -o pid,rss,cmd -p $(pidof webserv)' (Refreshes every 1 second) 
```
if the rss sudenly increased, it indicates memory leaks

## check hanging connections
While running the siege, open another terminal and run:
```bash
        watch -n 1 "ss -tanp | grep webserv"
```
If the number of open or stuck connections keeps growing and never drops, that’s a red flag for hanging connections or resource leaks.



#  bonus part
