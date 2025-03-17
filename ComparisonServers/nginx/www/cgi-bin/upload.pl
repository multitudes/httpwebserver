#!/usr/bin/perl

use strict;
use warnings;
use CGI;

my $cgi = CGI->new();

if (defined $cgi->param('file')) {
    my $uploaded_file = $cgi->upload('file');

    if (defined $uploaded_file) {
        open(my $fh, ">", "/path/to/uploads/" . $uploaded_file->filename) or die "Could not open file for writing: $!";
        binmode($fh);
        print $fh $uploaded_file->data;
        close($fh);

        print $cgi->header(-status => 200, -type => 'text/plain');
        print "File uploaded successfully!\n";
    } else {
        print $cgi->header(-status => 500);
        print "Error uploading file.\n";
    }
} else {
    print $cgi->header(-status => 400);
    print "No file selected.\n";
}