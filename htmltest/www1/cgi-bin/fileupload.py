#!/usr/bin/env python3

import os
import sys
import signal
from urllib.parse import quote


# Get the upload directory from the environment variable
UPLOAD_DIR = os.getenv('UPLOAD_DIR', './html/www1/upload')

# Handle broken pipes gracefully
signal.signal(signal.SIGPIPE, signal.SIG_DFL)  

def main():
    try:
        _ = sys.stdin.read()
        if not os.path.exists(UPLOAD_DIR):
            os.makedirs(UPLOAD_DIR)
            
        # Start the HTML output
        html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Webserv File Upload Form</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
    <script>
        function validateForm() {
            var x = document.forms["uploadForm"]["file"].value;
            if (x == "") {
                alert("No file selected. Please select a file to upload.");
                return false;
            }
        }
        function checkFiles() {
            var checkboxes = document.querySelectorAll('input[name="delete_files"]:checked');
            var deleteButton = document.getElementById("deleteButton");
            deleteButton.disabled = checkboxes.length === 0;
        }
        function uncheckAll() {
            let inputs = document.querySelectorAll('.check');
            for (let i = 0; i < inputs.length; i++) {
                inputs[i].checked = false;
            }
        }
        window.onload = function() {
            document.getElementById("file").value = "";
            document.getElementById("uploadButton").disabled = true;
            document.getElementById("file").addEventListener("change", function() {
                document.getElementById("uploadButton").disabled = this.value === "";
            });
            uncheckAll();
            checkFiles();
        }
    </script>
</head>
<body>
    <h1>File Upload</h1>
    <div class="box">
        <form name="uploadForm" action="/cgi/upload.py" method="POST" enctype="multipart/form-data" onsubmit="return validateForm()">
            <label for="file">Select file:</label>
            <input type="file" id="file" name="file">
            <button type="submit" id="uploadButton" disabled>Upload</button>
        </form>
        <p>Supported file types: .jpg, .png, .pdf, .txt</p>
    </div>
    <div class="file-list">
        <h2>Uploaded Files</h2>
        <form action="/cgi/delete.py" method="POST">
            <ul class="file-list-ul">
        """

        # List files in the upload directory
        files = [f for f in os.listdir(UPLOAD_DIR) if os.path.isfile(os.path.join(UPLOAD_DIR, f))]
        for filename in files:
            html_content += f"""
<li>
    <input type="checkbox" class="check" name="delete_files" value="{filename}" onclick="checkFiles()">
    <a href="/upload/{quote(filename)}" target="_blank">{filename}</a>
</li>
            """

        # End the HTML output
        if files:
            html_content += """
            </ul>
            <button type="submit" id="deleteButton">Delete Selected Files</button>
        </form>
    </div>
</body>
</html>
            """
        else:
            html_content += """
            </ul>
            <button type="submit" id="deleteButton" disabled>Delete Selected Files</button>
        </form>
        <p>No files available for deletion.</p>
    </div>
</body>
</html>
            """

        # Print the HTTP headers
        print("HTTP/1.1 200 OK")
        print("Content-Type: text/html")
        print(f"Content-Length: {len(html_content)}")
        print()
        # Print the HTML content
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