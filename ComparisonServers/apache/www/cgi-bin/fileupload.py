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
        # Print the HTTP headers
        print("Content-Type: text/html")
        print()  # Blank line required to end headers

        # Start the HTML output
        print("""
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
                <form name="uploadForm" action="/cgi-bin/upload.py" method="POST" enctype="multipart/form-data" onsubmit="return validateForm()">
                    <label for="file">Select file:</label>
                    <input type="file" id="file" name="file">
                    <button type="submit" id="uploadButton" disabled>Upload</button>
                </form>
                <p>Supported file types: .jpg, .png, .pdf, .txt</p>
            </div>
            <div class="file-list">
                <h2>Uploaded Files</h2>
                <form action="/cgi-bin/delete.py" method="POST">
                    <ul class="file-list-ul">
        """)

        # List files in the upload directory
        files = [f for f in os.listdir(UPLOAD_DIR) if os.path.isfile(os.path.join(UPLOAD_DIR, f))]
        for filename in files:
            print(f"""
                <li>
                    <input type="checkbox" class="check" name="delete_files" value="{filename}" onclick="checkFiles()">
                    <a href="/uploads/{filename}" target="_blank">{filename}</a>
                </li>
            """)

        # End the HTML output
        if files:
            print("""
                    </ul>
                    <button type="submit" id="deleteButton">Delete Selected Files</button>
                </form>
            </div>
        </body>
        </html>
            """)
        else:
            print("""
                    </ul>
                    <button type="submit" id="deleteButton" disabled>Delete Selected Files</button>
                </form>
                <p>No files available for deletion.</p>
            </div>
        </body>
        </html>
            """)

    except Exception as e:
        # Print error message
        print("Content-Type: text/html")
        print()
        print(f"<html><body><h1>Error</h1><p>{e}</p><form action='/cgi-bin/fileupload.py' method='get'><button type='submit'>Go Back</button></form></body></html>")

if __name__ == "__main__":
    main()