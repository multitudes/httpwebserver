#!/usr/bin/env python3

import cgi
import cgitb

# Enable CGI traceback for debugging
cgitb.enable()

# Print the HTTP headers

print("Content-Type: text/html")  # HTML is following
print()  # Blank line required to end headers

# Parse the form data
form = cgi.FieldStorage()

# Check if the required fields are present
if "name" not in form or "addr" not in form:
    print("<H1>Error</H1>")
    print("Please fill in the name and addr fields.")
else:
    # Display the submitted data
    print("<p>name:", form["name"].value)
    print("<p>addr:", form["addr"].value)