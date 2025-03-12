## Three-Way Handshake in TCP
The three-way handshake protocol is part of the **Transport Layer** in the OSI (Open Systems Interconnection) model. Specifically, it is used by the Transmission Control Protocol (TCP) to establish a reliable connection between a client and a server.  

It involves three steps:
1. **SYN (Synchronize)**: The client sends a TCP segment with the SYN flag set to the server to initiate a connection.
2. **SYN-ACK (Synchronize-Acknowledge)**: The server responds with a TCP segment that has both the SYN and ACK flags set, acknowledging the client's request and indicating its willingness to establish a connection.
3. **ACK (Acknowledge)**: The client sends a TCP segment with the ACK flag set, acknowledging the server's response. At this point, the connection is established, and data transfer can begin.

### Diagram of the Three-Way Handshake
When a TCP connection is established, both the client and the server choose an initial sequence number (ISN). These ISNs are chosen randomly to minimize the risk of sequence number prediction attacks. 
#### client sends SYN 
The client sends a TCP segment with the SYN flag set and its chosen ISN. Let's assume the client's ISN is x.
#### the server responds with SYN-ACK 
The server responds with a TCP segment that has both the SYN and ACK flags set. The server chooses its own ISN, let's say y.
The ACK flag acknowledges the client's SYN segment by setting the acknowledgment number to x + 1 (the client's ISN plus one).
#### client sends ACK
The client sends a TCP segment with the ACK flag set, acknowledging the server's SYN-ACK segment.
The acknowledgment number is set to y + 1 (the server's ISN plus one).
The sequence number is set to x + 1 (the client's ISN plus one), indicating that the client is ready to send data starting from this sequence number.
```
Client                                    Server
  |                                          |
  | ---- SYN, Seq = x             ---------->|
  |                                          |
  | <--- SYN, ACK, Seq = y, Ack = x + 1 ---> |
  |                                          |
  | ---- ACK, Seq = x + 1, Ack = y + 1 ----> |
  |                                          |
```

- **Layer**: The three-way handshake protocol is part of the **Transport Layer** in the OSI model.
- **Protocol**: It is used by the Transmission Control Protocol (TCP) to establish a reliable connection.

More about tcp is in the RFC 793 in links section.  
The above works in case of no data. If data is sent, the sequence number is incremented by the number of bytes sent...  

### Closing a TCP Connection
Involves a 4 way handshake.
- The client sends a TCP segment with the FIN flag set to the server to close the connection.
- The server responds with a TCP segment that has the ACK flag set to acknowledge the client's FIN segment.
- The server then sends its own FIN segment to the client to close the connection from its side.
- The client acknowledges the server's FIN segment with an ACK segment, and the connection is closed.