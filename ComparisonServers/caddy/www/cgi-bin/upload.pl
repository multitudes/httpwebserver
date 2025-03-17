#!/usr/bin/perl
use strict;
use warnings;
use CGI;
use CGI::Carp qw(fatalsToBrowser warningsToBrowser);
use File::Copy;

# Enable debugging
$CGI::POST_MAX = 1024 * 1024 * 10;  # 10MB max
$CGI::DISABLE_UPLOADS = 0;

# Create CGI object
my $q = CGI->new(\&CGI::Carp::real_carp);

# Print HTTP headers first
print $q->header('text/html');

# Start HTML output
print $q->start_html('File Upload Result');

eval {
    my $fh = $q->upload('file');
    
    if (!$fh) {
        print $q->p("No file uploaded: " . $q->cgi_error);
        exit;
    }

    my $filename = $q->param('file');
    my $upload_dir = "/var/www/html/uploads";
    
    # Create directory if it doesn't exist
    unless (-d $upload_dir) {
        mkdir $upload_dir or die "Cannot create directory: $!";
    }
    
    if ($filename =~ /\.(jpg|jpeg|png|gif|pdf|txt)$/i) {
        my $upload_filehandle = $q->upload('file');
        my $upload_path = "$upload_dir/$filename";
        
        open(my $out, '>', $upload_path) or die "Cannot open file: $!";
        binmode $out;
        
        while (<$upload_filehandle>) {
            print $out $_;
        }
        close $out;
        
        print $q->h2('Upload Successful');
        print $q->p("File '$filename' has been uploaded.");
    } else {
        print $q->h2('Invalid File Type');
        print $q->p('Only jpg, jpeg, png, gif, pdf, and txt files are allowed.');
    }
};

if ($@) {
    print $q->h2('Error');
    print $q->p("An error occurred: $@");
}

print $q->end_html;