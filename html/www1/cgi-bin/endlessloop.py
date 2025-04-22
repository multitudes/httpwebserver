#!/usr/bin/env python3

import sys
import time

def main():
    # First, output a proper CGI header so browsers know it's started
    print("Content-Type: text/plain")
    print()  # Empty line to separate headers from body
    print("Starting endless loop...", flush=True)  # flush ensures this gets sent immediately
    
    # Make the file executable
    sys.stdout.flush()  # Force output to be sent
    
    # Now go into endless loop
    count = 0
    while True:
        # Print something every 5 seconds to see it's still alive
        if count % 5 == 0:
            print(f"Still running... {count} seconds", flush=True)
        time.sleep(1)
        count += 1

if __name__ == "__main__":
    main()