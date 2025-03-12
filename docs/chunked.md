# Chunked Transfer Encoding

A chunked HTTP request uses the `Transfer-Encoding: chunked` header to indicate that the message body is sent in chunks. 
This request will not have a `Content-Length` header because it is used in cases where the sender doesn't know the length of the body beforehand.  
Each chunk is preceded by its size in hexadecimal format, followed by a CRLF (carriage return and line feed). The end of the message is indicated by a chunk of size zero.

### Example of a Chunked HTTP Request

Here is an example of a chunked HTTP request:

```http
POST /upload HTTP/1.1
Host: example.com
Transfer-Encoding: chunked
Content-Type: text/plain

7\r\n
Mozilla\r\n
9\r\n
Developer\r\n
7\r\n
Network\r\n
0\r\n
\r\n
```

### Explanation:
1. **Headers**: The request starts with the usual HTTP headers, including `Transfer-Encoding: chunked`.
2. **Chunks**: Each chunk starts with its size in hexadecimal format, followed by a CRLF (`\r\n`), the chunk data, and another CRLF.
   - `7\r\nMozilla\r\n`: A chunk of size 7 (hexadecimal) containing "Mozilla".
   - `9\r\nDeveloper\r\n`: A chunk of size 9 (hexadecimal) containing "Developer".
   - `7\r\nNetwork\r\n`: A chunk of size 7 (hexadecimal) containing "Network".
3. **End of Message**: The end of the message is indicated by a chunk of size zero (`0\r\n\r\n`).

### Example of a Chunked HTTP Response

Here is an example of a chunked HTTP response:

```http
HTTP/1.1 200 OK
Content-Type: text/plain
Transfer-Encoding: chunked

7\r\n
Mozilla\r\n
9\r\n
Developer\r\n
7\r\n
Network\r\n
0\r\n
\r\n
```

### Explanation:
1. **Headers**: The response starts with the usual HTTP headers, including `Transfer-Encoding: chunked`.
2. **Chunks**: Each chunk starts with its size in hexadecimal format, followed by a CRLF (`\r\n`), the chunk data, and another CRLF.
   - `7\r\nMozilla\r\n`: A chunk of size 7 (hexadecimal) containing "Mozilla".
   - `9\r\nDeveloper\r\n`: A chunk of size 9 (hexadecimal) containing "Developer".
   - `7\r\nNetwork\r\n`: A chunk of size 7 (hexadecimal) containing "Network".
3. **End of Message**: The end of the message is indicated by a chunk of size zero (`0\r\n\r\n`).

## Content Types
Chunked transfer encoding can be used with any content type. However, it is commonly used with content types that involve large or dynamically generated content, such as:

1. **Text/HTML**: For dynamically generated HTML content.
   ```http
   Content-Type: text/html
   ```

2. **Text/Plain**: For plain text content.
   ```http
   Content-Type: text/plain
   ```

3. **Application/JSON**: For JSON data, often used in APIs.
   ```http
   Content-Type: application/json
   ```

4. **Application/XML**: For XML data.
   ```http
   Content-Type: application/xml
   ```

5. **Application/Octet-Stream**: For binary data.
   ```http
   Content-Type: application/octet-stream
   ```

6. **Image/PNG**: For PNG images.
   ```http
   Content-Type: image/png
   ```

7. **Image/JPEG**: For JPEG images.
   ```http
   Content-Type: image/jpeg
   ```

8. **Video/MP4**: For MP4 video files.
   ```http
   Content-Type: video/mp4
   ```

Here is an example of a chunked HTTP response with JSON content:

```http
HTTP/1.1 200 OK
Content-Type: application/json
Transfer-Encoding: chunked

1E\r\n
{"name":"John","age":30,"city":"New York"}\r\n
0\r\n
\r\n
```

## How We Deal with Chunked Responses in Our Server

We do not handle streaming, so we do not handle chunked requests. File uploads are handled as multipart requests in the CGI server. There is no need for us to handle chunked requests at this time. The CGI will not handle them automatically like multipart requests, so we would need to dechunk them and assemble the body, but there is no use case for it yet.

A chunked request should return a `501 Not Implemented` response.

## Testing
A way to test is to use `curl` with the `--data-binary` flag to send a chunked request. Here is an example:

```sh
curl -X POST http://localhost:4242/upload \
  -H "Transfer-Encoding: chunked" \
  --data-binary $'7\r\nMozilla\r\n9\r\nDeveloper\r\n7\r\nNetwork\r\n0\r\n\r\n'
```

Or with `nc`:

```sh
echo -en "POST /upload HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n7\r\nMozilla\r\n9\r\nDeveloper\r\n7\r\nNetwork\r\n0\r\n\r\n" | nc localhost 4244
```