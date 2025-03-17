# Use xcaddy to build Caddy with CGI support
FROM caddy:2.8-builder-alpine AS builder
RUN xcaddy build --with github.com/aksdb/caddy-cgi/v2@v2.2.5

# Final stage
FROM alpine:3.18

# Copy the custom build of Caddy
COPY --from=builder /usr/bin/caddy /usr/bin/caddy

# Install Perl and CGI module for handling uploads
RUN apk update && \
    apk add --no-cache perl perl-cgi && \
    rm -rf /var/cache/apk/*

# Copy Caddyfile
COPY ./Caddyfile /etc/caddy/Caddyfile

# Copy website files to the correct location
COPY ./www/ /var/www/html/

# Make sure CGI directory exists and has correct permissions
RUN mkdir -p /var/www/html/cgi-bin && \
    chmod 755 /var/www/html/cgi-bin && \
    chmod 755 /var/www/html/cgi-bin/*.pl && \
    chown -R nonroot:nonroot /var/www/html

# Create nonroot user and group
RUN addgroup -S nonroot || true && \
    adduser -S nonroot -G nonroot || true

# Create uploads directory with correct user/group
RUN mkdir -p /var/www/html/uploads && \
    chown -R nonroot:nonroot /var/www/html/uploads && \
    chmod 755 /var/www/html/uploads

# Make CGI scripts executable
RUN chmod +x /var/www/html/cgi-bin/*.pl

# Expose the port
EXPOSE 4244

# Caddy runs as non-root by default
USER nonroot

# Start Caddy
CMD ["caddy", "run", "--config", "/etc/caddy/Caddyfile", "--adapter", "caddyfile"]

# run like 
# docker build -t test .
# docker run --rm --name test_server -p 4244:4244 test 