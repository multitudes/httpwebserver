#!/bin/perl
# filepath = /home/lbrusa/sgoinfre/42-Webserv/cgi-bin/hello.pl

print "HTTP/1.1 200 OK\r\n";
print "Content-type: text/html\r\n";
print "Content-Length: 17\r\n";
print "\r\n";
print "Hello, CGI-World!";