#!/bin/sh

echo "[nginx config] Configuring Nginx..."

# Replace placeholders with environment variable values
envsubst 'localhost' < /etc/nginx/nginx.conf.template > /etc/nginx/nginx.conf

cat /etc/nginx/nginx.conf

echo "creating a new user"
# create a user www and change the ownership of the files in the /run/nginx/ directory
adduser -D -g 'www' www &&\
chown -R www:www /run/nginx/ &&\
chown -R www:www /var/www/html/
mkdir -p /var/www/html/cgi-bin && \
chmod 755 /var/www/html/cgi-bin && \
chown www:www /var/www/html/cgi-bin

# Start Nginx
echo "[nginx config] Starting Nginx..."
# getent passwd www
exec nginx -g "daemon off;"