

# Some Definitions
- **Socket**: A socket is an endpoint for communication between two machines over a network. It can be used to send and receive data, establish connections, and perform other network-related tasks. Sockets are identified by an IP address and a port number and are in the OSI layer 4 (Transport Layer) of the network stack.
- **Port**: A port is a communication endpoint in an operating system that allows multiple processes to use the same network interface. Ports are identified by numbers ranging from 0 to 65535 and are used to direct network traffic to specific applications or services running on a machine and are in the OSI layer 4 (Transport Layer) of the network stack.
- **HTTP Methods**: HTTP methods are verbs that define the actions that a client can perform on a resource. The most common HTTP methods are GET, POST, PUT, DELETE, HEAD, and OPTIONS. Each method has a specific purpose and behavior, such as retrieving data (GET), submitting data (POST), updating data (PUT), deleting data (DELETE), and more.
- **HTTP Headers**: HTTP headers are additional information sent with an HTTP request or response that provides metadata about the message. Headers include details like the content type, content length, cache control, and more. They help the client and server understand how to process the message and handle the data.
- **HTTP Status Codes**: HTTP status codes are three-digit numbers returned by a server in response to an HTTP request. They indicate the outcome of the request, such as success, redirection, client errors, server errors, and more. Common status codes include 200 (OK), 404 (Not Found), 500 (Internal Server Error), and 301 (Moved Permanently).
- **HTTP Cookies**: HTTP cookies are small pieces of data stored on a client's device by a web server. Cookies are used to track user sessions, store user preferences, and personalize the user experience. They are sent with HTTP requests and responses and help maintain state between requests.
- **HTTP Session**: An HTTP session is a series of interactions between a client and a server that occur within a specific time frame. Sessions are used to maintain stateful communication between the client and server, such as tracking user logins, shopping carts, and other user-specific data.
- **HTTP Request**: An HTTP request is a message sent by a client to a server to request a resource or perform an action. Requests include details like the HTTP method, URL, headers, and body. The server processes the request and sends back an HTTP response.
- **HTTP Response**: An HTTP response is a message sent by a server to a client in response to an HTTP request. Responses include details like the status code, headers, and body. The client processes the response and displays the content to the user.
- **HTTP Body**: The body of an HTTP message contains the data being sent between the client and server. In requests, the body may include form data, file uploads, or other content. In responses, the body contains the requested resource, such as a web page, image, or file. The body can be in various formats, such as plain text, JSON, XML, or binary data. Also it can be chunked or multipart.
- **TLS** We do not support it but TLS (Transport Layer Security) is a cryptographic protocol designed to provide secure communication over a computer network. It is widely used to secure communications between web browsers and servers, ensuring data privacy and integrity. TLS is the successor to SSL (Secure Sockets Layer).
- **MIME** MIME stands for Multipurpose Internet Mail Extensions. It's a standard that defines how different types of data, such as text, images, audio, and video, are encoded and transmitted over the internet.  
MIME types are used to specify the content type of a file or data. This information helps applications like web browsers, email clients, and servers to handle and display the data correctly.  
Here are some common MIME types:  
`text/plain: Plain text (e.g., text files, email messages)`  
`text/html: HTML (Hypertext Markup Language) for web pages`  
`text/css: CSS (Cascading Style Sheets) for styling web pages`  
`text/javascript: JavaScript for dynamic web content`  
`image/jpeg: JPEG (Joint Photographic Experts Group) images`  
`image/png: PNG (Portable Network Graphics) images`  
`image/gif: GIF (Graphics Interchange Format) images`  
`audio/mpeg: MP3 audio files`  
`audio/wav: WAV audio files`  
`video/mp4: MP4 video files`  
`video/webm: WebM video files`  
MIME types are typically specified in the HTTP headers of a request or response. For example, a web server might send a response with a Content-Type header like this:  
`Content-Type: image/jpeg`

- **URI or URL** URL and URI are closely related, but there are some key differences: URI (Uniform Resource Identifier) is a more general term that encompasses any resource on the internet.  
Examples: `mailto:someone@example.com`, `tel:1234567890`, `urn:isbn:0-321-14652-5`
The URL (Uniform Resource Locator) is a specific type of URI that provides the location of a resource on the internet.
Typically used for web-based resources, such as web pages, images, and files.  
Examples: `http://www.example.com`, `https://images.example.com/image.jpg`  
- **URN** URN (Uniform Resource Name) is another type of URI that is used to identify a resource by name in a persistent and location-independent manner.
Examples: `urn:isbn:0-321-14652-5`, `urn:uuid:6e8bc430-9c3a-11d9-9669-0800200c9a66`	
- **URL** URL (Uniform Resource Locator) is a specific type of URI that provides the location of a resource on the internet.
Examples: `http://www.example.com`, `https://images.example.com/image.jpg`
- **URI Scheme** URI schemes are the first part of a URI that specifies the protocol or method used to access the resource.
Examples: `http://`, `https://`, `ftp://`, `mailto:`, `tel:`
- **Query String** A query string is a part of a URL that contains data to be passed to the server as key-value pairs.
Example: `http://www.example.com/search?q=query&limit=10`
- **Fragment Identifier** A fragment identifier is a part of a URL that specifies a specific section or anchor within a web page.
Example: `http://www.example.com/page#section1`
- **Relative URL** A relative URL is a URL that specifies the location of a resource relative to the current page or base URL.
Example: `../images/logo.png`, `page2.html`
- **Absolute URL** An absolute URL is a URL that specifies the complete location of a resource, including the protocol and domain.
Example: `http://www.example.com/images/logo.png`
- **Protocol-relative URL** A protocol-relative URL is a URL that specifies the location of a resource relative to the current protocol.
Example: `//cdn.example.com/script.js`
- **TCP/IP** TCP/IP (Transmission Control Protocol/Internet Protocol) is a suite of communication protocols used to connect devices over the internet. It provides reliable, end-to-end communication between devices by breaking data into packets and routing them across networks. TCP/IP includes protocols like TCP, IP, UDP, and ICMP.
more about the TCP handshake here: [tcp-handshake.md](docs/tcp-handshake.md)
