#!/usr/bin/env python3

import signal
import sys
import time
import os # Optional: for printing PID

# Define the signal handler function
def handle_sigterm(signum, frame):
    print(f"PID {os.getpid()}: Received SIGTERM ({signum}), exiting gracefully.", file=sys.stderr)
    sys.exit(0) # Exit with success code

def main():
    # Register the signal handler for SIGTERM
    signal.signal(signal.SIGTERM, handle_sigterm)

    print(f"PID {os.getpid()}: Endless loop script started. Send SIGTERM to exit.", file=sys.stderr)

    while True:
        # Introduce a small sleep to allow signal handling
        # Without this, the tight 'continue' loop might prevent
        # the signal handler from running promptly.
        try:
            time.sleep(0.1) # Sleep for 100 milliseconds
            # 'continue' is now effectively replaced by the loop structure
        except KeyboardInterrupt:
            # Also handle Ctrl+C nicely if run interactively
            print(f"\nPID {os.getpid()}: Received KeyboardInterrupt, exiting.", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
