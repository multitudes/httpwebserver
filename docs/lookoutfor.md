# Things to look out for - design choices

In this project we will have to: 

- decide which function to choose for server multiplexing.
- if there is an error by send, read, write or recv we need to close theconnection. The sys calls all need guards.
- Check the HTTP response status codes
- we need to be able to setup servers with differe ports.
- set up servers with different hotnames.
- default error pages. What happens if we dont get 404?
- what happens when the body is shorter or longer than the body limit? (content-length)
- The config file needs to be able to setup routes to different directories
- setup a default file to be served when a directory is requested
- setup a list of method accepted by the server for a certain route
- Implement GET POST DELETE 
- UNKNOWN requests dont crash the server. What the minimum we will accept as header info?
- Implement upload / download files
- GCI should be run in correct directory with relative path access
- CGI should work with get and post methods
- We use a browser to test the server
- try wrong url and access codes
- list a directory
- redirect a URL

- in the configuration should not be possible to select the same port multiple times.
- test the the config to use different ports and websites
- launch more than one server with different config and same ports
- monitor the process memory usage. It should not grow esponentially
- use the siege program to test

## bonus
- cookies and session
- more than one CGI script

## testing some of the above
### curl
Curl can be used in different ways and it is a powerful tool on the command line.
```bash
curl -X POST -H "Content-Type:plain/text" --data "BODY IS HERE" <URL>
```
The `curl` command in this case is used to send an HTTP POST request with a specified content type and body data:

- **`curl`**: is a command-line tool for transferring data with URLs.
- **`-X POST`**: Specifies the HTTP method to use, in this case, POST.
- **`-H "Content-Type:plain/text"`**: Sets the HTTP header `Content-Type` to `plain/text`. This indicates the media type of the resource being sent.
- **`--data "BODY IS HERE"`**: Specifies the data to be sent in the body of the POST request.

Here is an example of how you might use this command to send a POST request to a specific URL:

```sh
curl -X POST -H "Content-Type:plain/text" --data "BODY IS HERE" http://example.com/endpoint
```

### Explanation

- **`-X POST`**: Tells `curl` to use the POST method.
- **`-H "Content-Type:plain/text"`**: Sets the `Content-Type` header to `plain/text`, indicating that the body data is plain text.
- **`--data "BODY IS HERE"`**: Specifies the body data to be sent with the POST request. In this case, the body data is the string "BODY IS HERE".
- **`<URL>`**: The URL to which the POST request is sent. 

Also:
```bash
curl --resolve example.com:443:93.184.215.14 https://example.com
```

## Links
good for testing:  
https://httpbin.org 


