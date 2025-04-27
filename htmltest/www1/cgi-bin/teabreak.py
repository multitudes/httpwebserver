#!/usr/bin/env python3

import os

def main():
    # Start the HTML output
    html_content = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tea Time</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            font-family: Arial, sans-serif;
        }
        .container {
            text-align: center;
        }
        button {
            padding: 10px 20px;
            font-size: 16px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Tea Break</h1>
        <form name="coffee" action="/cgi/makecoffee.py" enctype="multipart/form-data" method="POST">
            <button type="submit">Make me a coffee?</button>
        </form>
    </div>
</body>
</html>
    """

    # Print the HTTP headers
    print("HTTP/1.1 200 OK")
    print("Content-Type: text/html")
    print(f"Content-Length: {len(html_content)}")
    print()  # End of headers
    # Print the HTML content
    print(html_content, end='')

if __name__ == "__main__":
    main()