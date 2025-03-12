## Allowed functions

Here at 42 we are allowed to use the following functions for this project:

Everything in C++ 98.
execve, dup, dup2, pipe, strerror, gai_strerror,
errno, dup, dup2, fork, socketpair, htons, htonl,
ntohs, ntohl, select, poll, epoll (epoll_create,
epoll_ctl, epoll_wait), kqueue (kqueue, kevent),

| Function | Description |
| -------- | ----------- |
| `execve()` | Executes a program specified by a filename. |
| `dup()` | Duplicates an open file descriptor. |
| `dup2()` | Duplicates an open file descriptor to a specified file descriptor. |
| `pipe()` | Creates a pipe, a unidirectional data channel. |
| `strerror()` | Returns a string describing the error code passed in the argument. |
| `gai_strerror()` | Returns a string describing an error code returned by the `getaddrinfo()` function. |
| `errno` | Contains the error code of the last failed function call. |
| `fork()` | Creates a new process by duplicating the calling process. |
| `socketpair()` | Creates a pair of connected sockets. |
| `htons()` | Converts a 16-bit host byte order to network byte order. |
| `htonl()` | Converts a 32-bit host byte order to network byte order. |
| `ntohs()` | Converts a 16-bit network byte order to host byte order. |
| `ntohl()` | Converts a 32-bit network byte order to host byte order. |
| `select()` | Monitors multiple file descriptors, waiting until one or more of the file descriptors become "ready" for some class of I/O operation. |
| `poll()` | Polls multiple file descriptors to see if I/O is possible on any of them. |
| `epoll()` | A scalable I/O event notification interface. |
| `epoll_create()` | Creates an epoll instance. |
| `epoll_ctl()` | Controls an epoll instance. |
| `epoll_wait()` | Waits for events on an epoll instance. |
| `kqueue()` | A scalable event notification interface. |
| `kevent()` | Controls the kernel event queue. |
| `socket()` | Creates an endpoint for communication and returns a file descriptor. |
| `accept()` | Accepts a new incoming connection on a socket. |
| `listen()` | Listens for incoming connections on a socket. |
| `send()` | Sends a message on a socket. |
| `recv()` | Receives a message from a socket. |
| `chdir()` | Changes the current working directory. |
| `bind()` | Binds a socket to an address. |
| `connect()` | Initiates a connection on a socket. |
| `getaddrinfo()` | Resolves the domain name into a set of socket addresses. |
| `freeaddrinfo()` | Frees the memory allocated for the linked list of addrinfo structures. |
| `setsockopt()` | Sets the socket options. |
| `getsockname()` | Returns the local address of the socket. |
| `getprotobyname()` | Returns a pointer to a structure containing the information about the protocol. |
| `fcntl()` | Performs various operations on file descriptors. |
| `close()` | Closes a file descriptor. |
| `read()` | Reads data from a file descriptor. |
| `write()` | Writes data to a file descriptor. |
| `waitpid()` | Waits for a specific child process to terminate. |
| `kill()` | Sends a signal to a process. |
| `signal()` | Sets a function to handle a signal. |
| `access()` | Checks the real user's permissions for the file. |
| `stat()` | Returns information about a file. |
| `open()` | Opens a file. |
| `opendir()` | Opens a directory. |
| `readdir()` | Reads a directory. |
| `closedir()` | Closes a directory. |

Some error codes to handle: 400,403,404,405,413,431,408 for timeout