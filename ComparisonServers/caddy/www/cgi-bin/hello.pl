#!/usr/bin/perl
use strict;
use warnings;
# filepath = /home/lbrusa/sgoinfre/42-Webserv/cgi-bin/hello.pl

my $message = "Hello, CGI-World!";
my $length = length($message);

print "Content-type: text/html\r\n";
print "Content-Length: $length\r\n";
print "\r\n";
print $message;