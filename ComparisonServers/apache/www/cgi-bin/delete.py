#!/usr/bin/env python3

import os
import cgi
import cgitb

cgitb.enable()  # Enable detailed error messages

UPLOAD_DIR = os.getenv('UPLOAD_DIR')
if not UPLOAD_DIR:
    raise RuntimeError("UPLOAD_DIR environment variable not set")


def main():
    try:
        # Parse the form data
        form = cgi.FieldStorage()

        # Check if any files were selected for deletion
        if "delete_files" not in form:
            print("Content-Type: text/html")
            print()  # End of headers
            print("""
            <!DOCTYPE html>
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
                <form action="/cgi-bin/fileupload.py" method="get"><button type="submit">Go Back</button></form>
            </body>
            </html>
            """)
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
            print("Content-Type: text/html")
            print()  # End of headers
            print("""
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>Webserv File Deletion</title>
                <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
                <link rel="stylesheet" type="text/css" href="/css/style.css">
            </head>
            <body>
            """)
            if deleted_files:
                print("<h1>Deleted Files</h1>")
                print("<ul>")
                for filename in deleted_files:
                    print(f"<li>{filename}</li>")
                print("</ul>")
            if not_found_files:
                print("<h1>Files Not Found</h1>")
                print("<ul>")
                for filename in not_found_files:
                    print(f"<li>{filename}</li>")
                print("</ul>")
            print('<form action="/cgi-bin/fileupload.py" method="get"><button type="submit">Go Back</button></form>')
            print("</body></html>")

    except Exception as e:
        # Print error message
        print("Content-Type: text/html")
        print()
        print(f"""
        <!DOCTYPE html>
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
            <form action='/cgi-bin/fileupload.py' method='get'><button type='submit'>Go Back</button></form>
        </body>
        </html>
        """)

if __name__ == "__main__":
    main()