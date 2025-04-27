#!/usr/bin/env python3

import os
import sys

def main():
    # Read the body from stdin if CONTENT_LENGTH is set
    body = ""
    content_length = os.environ.get('CONTENT_LENGTH', '0')

    # Ensure content_length is a valid integer
    try:
        content_length = int(content_length)
    except ValueError:
        content_length = 0

    if content_length > 0:
        body = sys.stdin.read(content_length)

    # Construct the HTML content
    message = "<html><head>"
    message += "<link rel=\"icon\" href=\"/favicon/favicon.ico\" type=\"image/x-icon\">"
    message += "<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\">"
    message += "</head><body>"
    message += "<h1>Hello, CGI-World!</h1>"
    message += "<h2>Environment Variables</h2>"
    message += "<ul>"
    message += f"<li>CONTENT_TYPE: {os.environ.get('CONTENT_TYPE', '')}</li>"
    message += f"<li>CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH', '')}</li>"
    message += f"<li>PATH_INFO: {os.environ.get('PATH_INFO', 'N/A')}</li>"
    message += f"<li>PATH_TRANSLATED: {os.environ.get('PATH_TRANSLATED', 'N/A')}</li>"
    message += f"<li>SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'N/A')}</li>"
    message += f"<li>SERVER_PROTOCOL: {os.environ.get('SERVER_PROTOCOL', 'N/A')}</li>"
    message += f"<li>REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'N/A')}</li>"
    message += f"<li>QUERY_STRING: {os.environ.get('QUERY_STRING', 'N/A')}</li>"
    message += f"<li>SERVER_SOFTWARE: {os.environ.get('SERVER_SOFTWARE', 'N/A')}</li>"
    message += f"<li>SERVER_NAME: {os.environ.get('SERVER_NAME', 'N/A')}</li>"
    message += f"<li>SERVER_PORT: {os.environ.get('SERVER_PORT', 'N/A')}</li>"
    message += f"<li>REMOTE_ADDR: {os.environ.get('REMOTE_ADDR', 'N/A')}</li>"
    message += f"<li>REMOTE_HOST: {os.environ.get('REMOTE_HOST', 'N/A')}</li>"
    message += f"<li>REMOTE_USER: {os.environ.get('REMOTE_USER', 'N/A')}</li>"
    message += f"<li>GATEWAY_INTERFACE: {os.environ.get('GATEWAY_INTERFACE', 'N/A')}</li>"
    message += f"<li>AUTH_TYPE: {os.environ.get('AUTH_TYPE', 'N/A')}</li>"
    message += f"<li>UPLOAD_DIR: {os.environ.get('HTTP_UPLOAD_DIR', 'N/A')}</li>"
    message += f"<li>SERVER_PORT: {os.environ.get('SERVER_PORT', 'N/A')}</li>"
    message += "</ul>"

    # Print the body if received
    if body:
        message += "<h2>Body Received</h2>"
        message += "<p>First 100 characters:</p>"
        message += f"<pre>{body[:100]}</pre>"

    message += "</body></html>"

    # Calculate the content length
    length = len(message)

    # Print the HTTP headers
    print("HTTP/1.1 200 OK")
    print("Content-Type: text/html")
    print(f"Content-Length: {length}")
    print()
    
    # Print the HTML content
    print(message, end='')

if __name__ == "__main__":
    main()