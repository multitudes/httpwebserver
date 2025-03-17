#!/usr/bin/env python3

import cgi
import os
import cgitb

cgitb.enable()  # Enable detailed error messages

UPLOAD_DIR = os.getenv('UPLOAD_DIR')
if not UPLOAD_DIR:
    raise RuntimeError("UPLOAD_DIR environment variable not set")


def main():
    try:
        # Parse the form data
        form = cgi.FieldStorage()

        # Check if a file was uploaded
        if "file" not in form:
            print("Content-Type: text/html")
            print()  # End of headers
            print("<h1>Error</h1>")
            print("<p>No file uploaded.</p>")
            print('<form action="/cgi-bin/fileupload.py" method="get"><button type="submit">Go Back</button></form>')
        else:
            file_item = form["file"]
            if file_item.file:
                # Get the filename
                filename = os.path.basename(file_item.filename)
                # Define the full path to save the file
                file_path = os.path.join(UPLOAD_DIR, filename)
                
                # Ensure the upload directory exists
                if not os.path.exists(UPLOAD_DIR):
                    os.makedirs(UPLOAD_DIR)
                
                # Save the file
                with open(file_path, "wb") as f:
                    f.write(file_item.file.read())
                
                # Redirect to the file upload form
                print("Status: 303 See Other")
                print("Location: /cgi-bin/fileupload.py")
                print()  # End of headers
            else:
                print("Content-Type: text/html")
                print()  # End of headers
                print("<h1>Error</h1>")
                print("<p>Error uploading file.</p>")
                print('<form action="/cgi-bin/fileuploadform.py" method="get"><button type="submit">Go Back</button></form>')
    
    except Exception as e:
        # Print error message
        print("Content-Type: text/html")
        print()
        print(f"<html><body><h1>Error</h1><p>{e}</p></body></html>")

if __name__ == "__main__":
    main()