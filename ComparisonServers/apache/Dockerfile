# Use the official Apache HTTP Server image from the Docker Hub
FROM httpd:2.4

# Install Perl and other necessary packages
RUN apt-get update && \
    apt-get install -y perl libcgi-pm-perl python3 && \
    # Create www-data user if it doesn't exist
    groupadd -r www-data || true && \
    useradd -r -g www-data www-data || true && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# add user and group
RUN groupadd -r www-data || true && \
	useradd -r -g www-data www-data || true
	
# Copy your custom Apache configuration file
COPY ./my-httpd.conf /usr/local/apache2/conf/httpd.conf


# Copy your setup script and website files
COPY ./tools/setup.sh /tmp/setup.sh
COPY ./www/ /usr/local/apache2/htdocs/

# Ensure the setup script and CGI scripts are executable
RUN chmod +x /tmp/setup.sh
RUN chmod +x /usr/local/apache2/htdocs/cgi-bin/*

# Create uploads directory and set permissions
RUN mkdir -p /usr/local/apache2/htdocs/uploads && \
    chown www-data:www-data /usr/local/apache2/htdocs/uploads && \
    chmod 755 /usr/local/apache2/htdocs/uploads

# Ensure the CGI scripts are owned by the correct user
RUN chown www-data:www-data /usr/local/apache2/htdocs/cgi-bin/*

# Run the setup script
# start apache running in the background
ENTRYPOINT [ "/tmp/setup.sh" ]

# run the container like 
# docker build -t test .
# docker run --rm --name test_server -p 4244:4244 test 