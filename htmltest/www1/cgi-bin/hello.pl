#!/usr/bin/env perl


use strict;
use warnings;

# Ricken's quotes collection
my @quotes = (
    "Bullies are bull and lies \n---\n",
    "A society with festering workers cannot flourish, just as a man with rotting toes cannot skip.\n---\n",
    "In the center of industry is dust.\n---\n",
    "A good person will follow the rules. A great person will follow himself.\n---\n",
    "What separates man from machine is that machines cannot think for themselves. \nAlso they are made of metal, whereas man is made of skin.\n---\n",
    "Before you criticize someone, you should walk a mile in their shoes. \nThat way when you criticize them, you are a mile away from them and you have their shoes.\n---\n",
    "If you go flying back through time, and you see somebody else flying forward into the future, \nit's probably best to avoid eye contact.\n---\n",
    
);

# Calculate content length first
my $content = <<"END_HTML";
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Ricken's Quotes</title>
    <link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">
    <link rel="stylesheet" type="text/css" href="/css/style.css">
</head>
<body>
    <h1>Ricken's Philosophical Quotes</h1>
END_HTML

foreach my $quote (@quotes) {
    $quote =~ s/\n/<br>/g;  # Convert newlines to HTML line breaks
    $content .= "    <div class=\"quote\">$quote</div>\n";
}

$content .= "</body>\n</html>";

# Calculate content length
my $content_length = length($content);

# Print HTTP headers
print "HTTP/1.1 200 OK\r\n";
print "Content-Type: text/html\r\n";
print "Content-Length: $content_length\r\n";
print "\r\n";  # End of headers

# Print HTML content
print $content;

