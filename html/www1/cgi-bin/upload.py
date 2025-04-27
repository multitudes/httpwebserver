#!/usr/bin/env python3

import cgi
import os
import cgitb
import urllib.parse
import mimetypes

cgitb.enable()  # Enable detailed error messages

UPLOAD_DIR = os.getenv('UPLOAD_DIR', './html/www1/upload')

def main():
    try:
		# Get the upload directory from environment variable
        # UPLOAD_DIR = os.getenv('UPLOAD_DIR')
        if not os.path.exists(UPLOAD_DIR):
            os.makedirs(UPLOAD_DIR)
        # cgi,maxlen = 100 * 1024 * 1024
        # Parse the form data
        form = cgi.FieldStorage()
    # except cgi.MaxSizeExceeded:
    #     print("File too large.")
    #     return
    
        # Check if a file was uploaded
        if "file" not in form:
            html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Upload Error</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Error</h1>
    <p>No file uploaded.</p>
    <form action="/cgi/fileupload.py" method="get"><button type="submit">Go Back</button></form>
</body>
</html>
            """
            print("HTTP/1.1 400 Bad Request")
            print("Content-Type: text/html")
            print(f"Content-Length: {len(html_content)}")
            print()  # End of headers
            print(html_content)
        else:
            file_item = form["file"]
            if file_item.file:
                # Get the filename
                filename = os.path.basename(file_item.filename)
                filename = urllib.parse.unquote(filename) # Decode URL-encoded characters
                
                # Get the file's content type
                content_type = file_item.type
                if not content_type:
                    # Try to guess the content type from the file extension
                    content_type, _ = mimetypes.guess_type(filename)
                
                # Optional: Validate file type if you want to restrict uploads
                allowed_types = ['application/pdf', 'image/jpeg', 'image/png', 'text/plain']  # Add your allowed types
                if content_type not in allowed_types:
                    html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Upload Error</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Error</h1>
    <p>Invalid file type. Only PDF, JPEG, and PNG files are allowed.</p>
    <form action="/cgi/fileupload.py" method="get"><button type="submit">Go Back</button></form>
</body>
</html>
                    """
                    print("HTTP/1.1 400 Bad Request")
                    print("Content-Type: text/html")
                    print(f"Content-Length: {len(html_content)}")
                    print()
                    print(html_content, end='')
                    return

                # Define the full path to save the file
                file_path = os.path.join(UPLOAD_DIR, filename)
                
                # Ensure the upload directory exists
                if not os.path.exists(UPLOAD_DIR):
                    os.makedirs(UPLOAD_DIR)
                
                # Save the file
                with open(file_path, "wb") as f:
                    f.write(file_item.file.read())
                
                # Redirect to the file upload form
                print("HTTP/1.1 303 See Other")
                print("Location: /cgi/fileupload.py")
                print("Content-Type: text/html")
                print("Content-Length: 0")
                print()  # End of headers
            else:
                html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Upload Error</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Error</h1>
    <p>Error uploading file.</p>
    <form action="/cgi/fileupload.py" method="get"><button type="submit">Go Back</button></form>
</body>
</html>
                """
                print("HTTP/1.1 400 Bad Request")
                print("Content-Type: text/html")
                print(f"Content-Length: {len(html_content)}")
                print()  # End of headers
                print(html_content, end='')
    
    except Exception as e:
        # Print error message
        error_content = f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Upload Error</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Error</h1>
    <p>{e}</p>
    <form action='/cgi/fileupload.py' method='get'><button type='submit'>Go Back</button></form>
</body>
</html>
        """
        print("HTTP/1.1 500 Internal Server Error")
        print("Content-Type: text/html")
        print(f"Content-Length: {len(error_content)}")
        print()  # End of headers
        print(error_content, end='')

if __name__ == "__main__":
    main()