#!/usr/bin/env python3

import sys
import os
import time

# Save the incoming request to a file
def handle_request():
    # Get stdin data
    input_data = sys.stdin.buffer.read()
    
    # Create a timestamped filename
    timestamp = int(time.time())
    filename = f"request_{timestamp}.txt"
    
    # Save request to file
    with open(filename, "wb") as f:
        f.write(input_data)
    
    # Generate a simple response
    response = f"""HTTP/1.1 200 OK
Content-Type: text/plain
Content-Length: 59

Request received and saved to {filename} ({len(input_data)} bytes)
"""
    
    # Write response to stdout
    sys.stdout.write(response)
    sys.stdout.flush()

if __name__ == "__main__":
    handle_request()