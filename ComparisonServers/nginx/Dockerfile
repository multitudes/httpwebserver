FROM alpine:3.20

RUN apk update && apk upgrade && \
    apk add --no-cache openssl nginx gettext \
    fcgiwrap spawn-fcgi && \
    rm -rf /var/cache/apk

WORKDIR /var/www/html/

# copy conf file and setup script
COPY ./nginx.conf /etc/nginx/nginx.conf.template
COPY ./tools/setup.sh /tmp/setup.sh
# copy the website
COPY ./www/ /var/www/html/

RUN chmod +x /tmp/setup.sh

# # Make sure your CGI scripts are executable
# RUN chmod +x /var/www/html/cgi-bin/*.cgi

# # Add a healthcheck
# HEALTHCHECK --interval=30s --timeout=3s \
#     CMD wget --quiet --tries=1 --spider http://localhost:4244 || exit 1

# start nginx running in the background
ENTRYPOINT [ "/tmp/setup.sh" ]

# run like 
# docker build -t test .
# docker run --rm --name test_server -p 4244:4244 test 
# get id with docker ps
# docker exec -it <id> /bin/sh