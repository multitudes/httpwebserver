#!/usr/bin/env python3

import os
import cgi
import cgitb

cgitb.enable()  # Enable detailed error messages

UPLOAD_DIR = os.getenv('UPLOAD_DIR', './html/www1/upload')

def main():
    try:
        
        # Parse the form data
        form = cgi.FieldStorage()

        # Check if any files were selected for deletion
        if "delete_files" not in form:
            html_content = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Deletion</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Error</h1>
    <p>No files selected for deletion.</p>
    <form action="/cgi/fileupload.py" method="get"><button type="submit">Go Back</button></form>
</body>
</html>"""
            print("HTTP/1.1 400 Bad Request")
            print("Content-Type: text/html")
            print(f"Content-Length: {len(html_content)}")
            print()  # End of headers
            print(html_content, end='')
        else:
            files_to_delete = form.getlist("delete_files")
            deleted_files = []
            not_found_files = []

            for filename in files_to_delete:
                file_path = os.path.join(UPLOAD_DIR, filename)
                if os.path.exists(file_path):
                    os.remove(file_path)
                    deleted_files.append(filename)
                else:
                    not_found_files.append(filename)

            # Print the result
            html_content = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Deletion</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>"""
            if deleted_files:
                html_content += "<h1>Deleted Files</h1><ul>"
                for filename in deleted_files:
                    html_content += f"<li>{filename}</li>"
                html_content += "</ul>"
            if not_found_files:
                html_content += "<h1>Files Not Found</h1><ul>"
                for filename in not_found_files:
                    html_content += f"<li>{filename}</li>"
                html_content += "</ul>"
            html_content += '<form action="/cgi/fileupload.py" method="get"><button type="submit">Go Back</button></form>'
            html_content += "</body></html>"

            print("HTTP/1.1 200 OK")
            print("Content-Type: text/html")
            print(f"Content-Length: {len(html_content)}")
            print()  # End of headers
            print(html_content, end='')

    except Exception as e:
        # Print error message
        error_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Deletion</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Error</h1>
    <p>{e}</p>
    <form action='/cgi/fileupload.py' method='get'><button type='submit'>Go Back</button></form>
</body>
</html>"""

        print("HTTP/1.1 500 Internal Server Error")
        print("Content-Type: text/html")
        print(f"Content-Length: {len(error_content)}")
        print()  # End of headers
        print(error_content, end='')

if __name__ == "__main__":
    main()