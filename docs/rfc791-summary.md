# INTERNET PROTOCOL

#                         DARPA INTERNET PROGRAM
#                         PROTOCOL SPECIFICATION

September 1981 (NB this RFC refers to the IP protocol only)

The Internet Protocol is designed for use in interconnected systems of 
packet-switched computer communication networks.  
The internet protocol implements two basic functions:  
- addressing 
- fragmentation.

The selection of a path for transmission is called **routing**.  
The internet protocol uses four key mechanisms in providing its service:  
- Type of Service, used to specify the treatment of the datagram during
its transmission through the internet system.    
- Time to Live is an upper bound on the time that a datagram may exist in the internet system.  
- Options are provisions for timestams, security information, and special routing.
- Header Checksum is used to detect errors in the header. 

The internet protocol is a connectionless protocol.  
Each packet is treated independently of all others.  
The internet protocol does not provide a reliable communication facility.  There are no acknowledgments either end-to-end or hop-by-hop.  
There is no error control for data, only a header checksum.  
There are no retransmissions.  
There is no flow control.

Errors detected may be reported via the Internet Control Message 
Protocol (ICMP) which is implemented in the internet protocol module.

   				 +------+ +-----+ +-----+     +-----+
                 |Telnet| | FTP | | TFTP| ... | ... |
                 +------+ +-----+ +-----+     +-----+
                       |   |         |           |
                      +-----+     +-----+     +-----+
                      | TCP |     | UDP | ... | ... |
                      +-----+     +-----+     +-----+
                         |           |           |
                      +--------------------------+----+
                      |    Internet Protocol & ICMP   |
                      +--------------------------+----+
                                     |
                        +---------------------------+
                        |   Local Network Protocol  |
                        +---------------------------+

We start with our application programs, such as Telnet, FTP, and TFTP.  
The sending application program prepares its data and calls on its local
internet module to send that data as a datagram and passes the destination address and other parameters as arguments of the call.  

The internet module prepares a datagram header (IP datagram) and attaches the data to it. The header would look like this:


0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  |Type of Service|          Total Length         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification        |Flags|      Fragment Offset    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Time to Live |    Protocol   |         Header Checksum       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source Address                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination Address                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options                    |    Padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

example of an IP header:
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Ver= 4 |IHL= 5 |Type of Service|        Total Length = 21      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      Identification = 111     |Flg=0|   Fragment Offset = 0   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   Time = 123  |  Protocol = 1 |        header checksum        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         source address                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      destination address                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     data      |
+-+-+-+-+-+-+-+-+
the internet header consists of five 32 bit words (5 x 4 octets), and the total length of the datagram is 21 octets. This datagram is a complete datagram (not a fragment).

NB 
Total length.
The number 576 is usually selected to allow a reasonable sized data block to be transmitted in addition to the required header information.  
For example, this size allows a data block of 512 octets plus 64 header octets to fit in a datagram.  The maximal internet header is 60 octets, and a typical internet header is 20 octets, allowing a margin for headers of higher level protocols.
The internet module determines a local network address for this internet address, in this case it is the address of a gateway.  

Header Checksum:  16 bits 
A checksum on the header only.  Since some header fields change (e.g., time to live), this is recomputed and verified at each point that the internet header is processed.  
The checksum field is the 16 bit one's complement of the one's
complement sum of all 16 bit words in the header.  For purposes of
computing the checksum, the value of the checksum field is zero.

the security field is considered now obsolete and is not used.

TTL:  8 bits
The time to live is an upper bound on the time that an internet datagram may exist.  It is max 255 seconds.  If the TTL field is set to zero the datagram is not forwarded.  The TTL field is decremented by one at each hop.

If the destination address or port does not correspond to any active application or user process, the IP layer generates an ICMP (Internet Control Message Protocol) error message (e.g., "Destination Unreachable") and sends it back to the sender. The datagram is then discarded. This behavior is still standard in modern IP networking.  

The datagram is then passed to the local network module, reponsible to handle at the link layer like ethernet...  The local network module attaches a local network header to the datagram and sends the result to the gateway host.  The local network header would look like this:

+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Source Address  | Destination Address |  Type | Data | CRC  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Source Address  | Destination Address |  Type | Data | CRC  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

The gateway host receives the datagram, strips off the local network header, and turns the datagram over to the internet module.  The internet module determines that the datagram is to be forwarded to another host in another network.  The internet module determines a local network address for the destination host.  It calls on the local network module for that network to send the datagram.
This local network interface creates a local network header and attaches the datagram sending the result to the destination host.  
At this destination host the datagram is stripped of the local net header by the local network interface and handed to the internet module.

The internet module determines that the datagram is for an application program in this host.  
It passes the data to the application program in response to a system call, passing the source address and other parameters as results of the call.  


Application                                           Application
   Program                                                   Program
         \                                                   /
       Internet Module      Internet Module      Internet Module
             \                 /       \                /
             LNI-1          LNI-1      LNI-2         LNI-2
                \           /             \          /
               Local Network 1           Local Network 2



TCP attaches headers, a port number, and a checksum to the data.  The port number is used to direct the data to the appropriate application program.  The checksum is used to detect errors in the data.  The TCP module is responsible for reliable communication.

# In simpler words
## TCP Interaction with IP
- Encapsulation: TCP segments (units of data) are encapsulated within IP packets. The TCP header and data form the payload of the IP packet.
- Addressing: IP handles the addressing and routing of packets between hosts. TCP relies on IP to deliver its segments to the correct destination.
- Checksum: While IP includes a checksum for its header, TCP includes a checksum for its entire segment (header and data) to ensure data integrity. (? check)

### Example of TCP/IP Interaction
- Application Layer: An application (e.g., a web browser) sends data to the TCP layer.
- TCP Layer: TCP breaks the data into segments, adds a TCP header (including a checksum), and passes the segments to the IP layer.
- IP Layer: IP puts each TCP segment in an IP packet, adds an IP header (including a checksum for the IP header), and sends the packets to the network.
- Network Transmission: The packets are transmitted over the network, potentially passing through multiple routers.
- Receiving Host: At the receiving host, the IP layer processes the IP packets, verifies the IP header checksum, and passes the payload (TCP segments) to the TCP layer.
- TCP Layer: TCP verifies the TCP checksum, reassembles the segments into the original data stream, and delivers the data to the application.

## Which fields are still relevant today
The IP header as defined in RFC 791 includes several fields, some of which have become less relevant or obsolete in modern networking due to changes in technology and the development of new protocols.  
Here are the fields in the IP header and their current relevance:

- IP Header Fields
Version: relevant. It indicates the version of the IP protocol (IPv4 or IPv6).

- IHL (Internet Header Length): relevant. It specifies the length of the IP header.

- Type of Service (ToS): While the field itself is still present, its interpretation has changed.This field has evolved into the Differentiated Services Code Point (DSCP) in modern networks.

Total Length: still relevant. It specifies the total length of the IP packet, including the header and data.  

Identification, Flags, Fragment Offset: These fields are still relevant for handling IP fragmentation. However, fragmentation is less common in modern networks due to Path MTU Discovery (PMTUD) and other techniques.

Time to Live (TTL): This field is still relevant. It specifies the maximum number of hops a packet can take before being discarded.

Protocol: This field is still relevant. It indicates the protocol used in the data portion of the IP packet (e.g., TCP, UDP).

Header Checksum: This field is still relevant for IPv4. However, it is not present in IPv6, which relies on checksums in higher-layer protocols.

Source Address: This field is still relevant. It specifies the IP address of the sender.

Destination Address: This field is still relevant. It specifies the IP address of the receiver.

Options and Padding: This field is largely obsolete. While the options field can still be used, it is rarely utilized in modern networks due to performance and security concerns. Many routers and devices ignore or drop packets with IP options.

Summary of Obsolete or Less Relevant Fields
Type of Service (ToS): The interpretation has changed to DSCP.
Identification, Flags, Fragment Offset: Fragmentation is less common due to PMTUD.
Header Checksum: Not present in IPv6.
Options and Padding: Largely obsolete and rarely used in modern networks.
Example of Modern IP Header Usage
Here is a simplified view of the IP header fields that are still relevant in modern IPv4 networks:
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Version|  IHL  | DSCP |ECN|          Total Length              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Identification        |Flags|      Fragment Offset    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Time to Live |    Protocol   |         Header Checksum       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Source Address                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Destination Address                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Options                    |    Padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+


# NB3
the byte order of transmission
APPENDIX B:  Data Transmission Order

The order of transmission of the header and data described in this
document is resolved to the octet level.  Whenever a diagram shows a
group of octets, the order of transmission of those octets is the normal
order in which they are read in English.  For example, in the following
diagram the octets are transmitted in the order they are numbered.


    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       1       |       2       |       3       |       4       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       5       |       6       |       7       |       8       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       9       |      10       |      11       |      12       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                      Transmission Order of Bytes