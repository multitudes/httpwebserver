#!/usr/bin/perl

use strict;
use warnings;
use CGI;
use File::Copy;

# Create CGI object
my $cgi = CGI->new;

UPLOAD_DIR = os.getenv('UPLOAD_DIR')
if not UPLOAD_DIR:
    raise RuntimeError("UPLOAD_DIR environment variable not set")


print "Content-type: text/html\n\n";
print "<html><body>";

# Get the uploaded file
my $upload = $cgi->upload('file');

if ($upload) {
    my $filename = $cgi->param('file');
    
    # Validate file type (same as your nginx config)
    if ($filename =~ /\.(jpg|jpeg|png|gif|pdf|txt)$/i) {
        # Ensure the upload directory exists
        if not os.path.exists(UPLOAD_DIR):
            os.makedirs(UPLOAD_DIR)
        
        # Full path for the uploaded file
        my $upload_path = "$UPLOAD_DIR/$filename";
        
        # Copy the uploaded file
        if (copy($upload, $upload_path)) {
            print "<h2>File uploaded successfully!</h2>";
            print "<p>File: $filename</p>";
            print "<p><a href='/'>Back to home</a></p>";
        } else {
            print "<h2>Error uploading file: $!</h2>";
        }
    } else {
        print "<h2>Error: Invalid file type</h2>";
        print "<p>Supported types: jpg, jpeg, png, gif, pdf, txt</p>";
    }
} else {
    print "<h2>Error: No file uploaded</h2>";
}

print "</body></html>";