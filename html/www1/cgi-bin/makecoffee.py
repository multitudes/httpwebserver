#!/usr/bin/env python3
import os

def main():
    # Path to your error page (adjust as needed)
    error_page_path = "html/www1/error_pages/418.html"
    
    try:
        # Read the error page file
        with open(error_page_path, "r") as file:
            error_content = file.read()
        
        # Serve the error page with 418 status
        print("HTTP/1.1 418 I'm a teapot")
        print("Content-Type: text/html")
        print(f"Content-Length: {len(error_content)}")
        print()  # End of headers
        print(error_content, end='')  # Send the HTML content
    
    except Exception as e:
        # Fallback if file can't be read
        print("HTTP/1.1 418 I'm a teapot")
        print("Content-Type: text/html")
        print()
        print("<html><body><h1>418 I'm a teapot</h1><p>Error: Could not read the teapot error page.</p></body></html>")

if __name__ == "__main__":
    main()