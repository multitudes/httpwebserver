# test

Testing put request to the dir uploads:
```
echo "Hello, this is a test file" > test.txt
curl -X PUT --data-binary @test.txt http://localhost:4244/uploads/test.txt

# To see the full HTTP response headers
curl -v -X PUT --data-binary @test.txt http://localhost:4244/uploads/test.txt

# To upload to a subdirectory (if allowed by your server config)
curl -X PUT --data-binary @test.txt http://localhost:4244/uploads/subfolder/test.txt

# To check if the file was uploaded successfully
curl -X GET http://localhost:4244/uploads/test.txt

```

And to delete 
```
curl -X DELETE http://localhost:4244/uploads/test.txt

# or verbose
curl -v -X DELETE http://localhost:4244/uploads/test.txt
```

Example usage and output

```
curl -v -X PUT --data-binary @test.txt http://localhost:4244/uploads/test.txt

* Host localhost:4244 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:4244...
* connect to ::1 port 4244 from ::1 port 51106 failed: Connection refused
*   Trying 127.0.0.1:4244...
* Connected to localhost (127.0.0.1) port 4244
> PUT /uploads/test.txt HTTP/1.1
> Host: localhost:4244
> User-Agent: curl/8.7.1
> Accept: */*
> Content-Length: 22
> Content-Type: application/x-www-form-urlencoded
>
* upload completely sent off: 22 bytes
< HTTP/1.1 201 Created
< Content-Length: 0
< Location: /uploads/test.txt
<
* Connection #0 to host localhost left intact
melvin@MutantBot ~ % curl -v -X DELETE http://localhost:4244/uploads/test.txt
* Host localhost:4244 was resolved.
* IPv6: ::1
* IPv4: 127.0.0.1
*   Trying [::1]:4244...
* connect to ::1 port 4244 from ::1 port 51118 failed: Connection refused
*   Trying 127.0.0.1:4244...
* Connected to localhost (127.0.0.1) port 4244
> DELETE /uploads/test.txt HTTP/1.1
> Host: localhost:4244
> User-Agent: curl/8.7.1
> Accept: */*
>
* Request completely sent off
< HTTP/1.1 204 No Content
< Content-Length: 0
< Location: /uploads/test.txt
<
* Connection #0 to host localhost left intact
```