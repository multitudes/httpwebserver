# apache

The subject of webserv says to look to nginx for inspiration but the nginx doesnt do cgi. Therefore I created a small docker container with apache in the test folder, in the ComparisonServers folder of the project.

Passing our html files to the container I made a cgi script that uploads a file to the server.

## CGI and Apache
To configure Apache to handle CGI scripts, you need to ensure that the CGI module is enabled and that the appropriate directory is configured to allow the execution of CGI scripts. Here are the key configuration directives and steps you need to include in your Apache configuration file:

### Enabling the CGI Module

Ensure that the CGI module is enabled. You can enable it using the following command:

```sh
sudo a2enmod cgi
```

### Configuring the CGI Directory

You need to configure a directory where your CGI scripts will be located. This directory should have the appropriate permissions and options set to allow the execution of CGI scripts.

### Example Configuration

Here is an example configuration that you can add to your Apache configuration file (e.g., `/etc/apache2/sites-available/000-default.conf`):

```apache
<VirtualHost *:80>
    ServerAdmin webmaster@localhost
    DocumentRoot /var/www/html

    # Configure the CGI directory
    ScriptAlias /cgi-bin/ /usr/lib/cgi-bin/

    <Directory "/usr/lib/cgi-bin">
        AllowOverride None
        Options +ExecCGI
        AddHandler cgi-script .cgi .pl
        Require all granted
    </Directory>

    ErrorLog ${APACHE_LOG_DIR}/error.log
    CustomLog ${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
```

### Explanation

1. **ScriptAlias**:
   - The `ScriptAlias` directive maps a URL path to a directory on the filesystem. In this example, cgi-bin is mapped to 

cgi-bin

.

2. **Directory Configuration**:
   - The `<Directory>` block configures the 

cgi-bin

 directory.
   - `AllowOverride None`: Disables `.htaccess` overrides in this directory.
   - `Options +ExecCGI`: Enables the execution of CGI scripts in this directory.
   - `AddHandler cgi-script .cgi .pl`: Specifies that files with the `.cgi` and `.pl` extensions should be treated as CGI scripts.
   - `Require all granted`: Allows access to the directory from all clients.

### Setting Executable Permissions

Ensure that your CGI scripts have executable permissions:

```sh
chmod +x /usr/lib/cgi-bin/your_script.cgi
```

### Restarting Apache

After making these changes, restart Apache to apply the configuration:

```sh
sudo systemctl restart apache2
```

### Accessing the CGI Script

You can access your CGI script through your web browser by navigating to the appropriate URL, such as:

```
http://localhost/cgi-bin/your_script.cgi
```

### Environment Variables Set by Apache

When Apache processes a CGI request, it sets several environment variables that your CGI script can access. Some of the key environment variables include:

- `CONTENT_TYPE`: The MIME type of the request body.
- `CONTENT_LENGTH`: The length of the request body.
- `PATH_INFO`: Extra path information following the script name.
- `PATH_TRANSLATED`: The translated version of `PATH_INFO`.
- `SCRIPT_NAME`: The name of the CGI script.
- `SERVER_PROTOCOL`: The name and version of the information protocol.
- `REQUEST_METHOD`: The method used to make the request (e.g., GET, POST).
- `QUERY_STRING`: The query string part of the URL.
- `SERVER_SOFTWARE`: The name and version of the server software.
- `SERVER_NAME`: The server's hostname or IP address.
- `SERVER_PORT`: The port number on which the request was received.
- `REMOTE_ADDR`: The IP address of the client making the request.
- `REMOTE_HOST`: The hostname of the client making the request.
- `REMOTE_USER`: The authenticated user making the request.
- `GATEWAY_INTERFACE`: The CGI specification version.
- `AUTH_TYPE`: The authentication method used.

The CGI script can access these environment variables to process the request and generate the appropriate response.

## the apache logs

As an example I looked up the logs I get from from docker for my apache config.

```
172.17.0.1 - - [26/Jan/2025:10:42:44 +0000] "GET / HTTP/1.1" 304 -
172.17.0.1 - - [26/Jan/2025:10:42:44 +0000] "GET /favicon/favicon.ico HTTP/1.1" 304 -
172.17.0.1 - - [26/Jan/2025:10:42:46 +0000] "GET /fileuploadform.html HTTP/1.1" 304 -
172.17.0.1 - - [26/Jan/2025:10:42:53 +0000] "POST /cgi-bin/upload.pl HTTP/1.1" 200 118
172.17.0.1 - - [26/Jan/2025:10:42:55 +0000] "GET /css/style.css HTTP/1.1" 304 -
172.17.0.1 - - [26/Jan/2025:10:42:55 +0000] "GET /css/style.css HTTP/1.1" 304 -
172.17.0.1 - - [26/Jan/2025:10:42:56 +0000] "GET /go HTTP/1.1" 301 234
172.17.0.1 - - [26/Jan/2025:10:42:56 +0000] "GET /here HTTP/1.1" 301 235
172.17.0.1 - - [26/Jan/2025:10:42:56 +0000] "GET /here/ HTTP/1.1" 304 -
```

The format is: `IP - - [DATE:TIME] "REQUEST" STATUS_CODE RESPONSE_SIZE`

1. `304` responses:
```
"GET / HTTP/1.1" 304 -
"GET /favicon/favicon.ico HTTP/1.1" 304 -
"GET /fileuploadform.html HTTP/1.1" 304 -
```
- `304` means "Not Modified"
- The `-` in size means no content was sent
- This happens when the browser has a cached version and sends an "If-Modified-Since" header
- Server confirms the cached version is still valid, so no need to resend the file

2. `200 118`:
```
"POST /cgi-bin/upload.pl HTTP/1.1" 200 118
```
- `200` means "OK" - successful request
- `118` is the size in bytes of the response
- This is your Perl script's response HTML (the success/error message)
- Small size suggests it's probably the "No file uploaded" message

3. `301` redirects:
```
"GET /go HTTP/1.1" 301 234
"GET /here HTTP/1.1" 301 235
```
- `301` means "Permanent Redirect"
- `234` and `235` are the sizes of the redirect response headers
- Shows your redirect chain: `/go` → `/here` → `/here/`
- The final `/here/` returns `304` because it's cached

The sequence shows:
1. User loaded the main page (cached)
2. Accessed upload form (cached)
3. Submitted form but probably without a file
4. Tried the redirect from `/go` to `/here`