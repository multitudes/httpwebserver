#!/bin/sh

# # Create the uploads directory if it doesn't exist
# mkdir -p /usr/local/apache2/htdocs/uploads

# # Set permissions for the uploads directory
# chown -R www-data:www-data /usr/local/apache2/htdocs/uploads
# chmod -R 755 /usr/local/apache2/htdocs/uploads

# Start Apache
echo "[apache config] Starting Apache..."
exec httpd -D FOREGROUND
