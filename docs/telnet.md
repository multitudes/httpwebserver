 ## using telnet to test

telnet is a useful tool for testing and debugging network services, providing remote access to systems, and interacting with network services.

```bash
telnet localhost 4244
```

The connection will be established and you will get a message from the server.

## Telnet and NC

- telnet:
Purpose: Primarily designed for remote terminal access and communication using the Telnet protocol.
Protocol: Uses the Telnet protocol, which is a text-based protocol for interactive communication.
Port: Typically uses port 23 by default, but can connect to any specified port.
Features: Provides a simple way to interact with remote servers, especially useful for testing and debugging text-based protocols like HTTP.
Limitations: Not secure (transmits data in plain text), limited to text-based communication, and less flexible compared to nc.

- nc (Netcat):
Purpose: A versatile networking utility for reading from and writing to network connections using TCP or UDP.
Protocol: Can use both TCP and UDP protocols.
Port: Can connect to any specified port.
Features: Highly flexible, can be used for port scanning, transferring files, creating simple servers, and more. Supports both text and binary data.
Security: More flexible and powerful than telnet, but also transmits data in plain text unless used with additional tools for encryption.

examples with nc
```bash
nc localhost 4244
# file transfer
nc -l 1234 > received_file.txt
# create a server
nc -l 4244
```