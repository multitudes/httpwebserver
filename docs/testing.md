# Testing

We used pytest to test the project as we developed features, both running locally and on the GitHub Actions platform.

## Table of Contents
- [Pytest](#pytest)
- [Netcat](#netcat)
- [GitHub Workflows](#github-workflows)
- [VS Code Debugger Setup](#setting-up-the-vs-code-debugger)
- [Curl Examples](#curl)

## Pytest
### Setting It Up
To test our webserver, we followed these steps:

1. Create a virtual environment for our Python tester
2. Set up fixtures that start and stop our server with different configurations
3. Write integration tests that validate server responses

Our test fixtures use subprocess to launch the server with different configuration files:

```python
@pytest.fixture(scope="function")
def webserver_normal_config():
    server = start_webserver("tests/config/default.conf")
    yield
    server.terminate()
```

Tests then use these fixtures to verify different aspects of the server functionality:

```python
def test_normal_config_upload(webserver_normal_config):
    """Test file upload (only works with normal config)"""
    test_file = 'tests/uploadtest.txt'
    upload_url = 'http://localhost:4244/upload/test.txt'
    # Test implementation...
```

## Netcat
For debugging, netcat (nc) and curl are very useful command-line tools.

HTTP headers always end with the sequence `\r\n\r\n`. This marks the end of the headers and the beginning of the body, if present. Typically, GET requests don't have a body. In POST requests, the Content-Length header specifies the length of the body.

### Basic Usage
You can connect to your server's port using netcat:

```bash
nc localhost 4243
```

However, to properly send HTTP headers with the CRLF sequence, it's easier to pipe a request to netcat:

```bash
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 4243
```

The `-e` option enables interpretation of backslash escapes, necessary for the `\r` and `\n` characters. You can also use `-n` with echo to avoid an extra newline at the end of the string.

### Testing Different Requests

Test a malformed request:
```bash
echo -e "\r\n\r\na" | nc localhost 4243
```

Test a POST request:
```bash
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 3\r\n\r\na=1" | nc localhost 4243
```

## GitHub Workflows

To set up CI/CD with GitHub workflows:

1. Create a folder: `.github/workflows`
2. Create a file: `ci.yml` with the following content:

```yml
# .github/workflows/ci.yml
name: CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v2
      with:
        submodules: true  

    - name: Set up Python
      uses: actions/setup-python@v2
      with:
        python-version: '3.11'

    - name: Install dependencies
      run: |
        python -m pip install --upgrade pip
        python -m pip install pytest

    - name: Build
      run: make

    - name: Run tests
      run: |
        make venv
        make test
```

After pushing this workflow to your repository, go to the Actions tab in GitHub to see it running.

## Setting Up the VS Code Debugger

### On macOS
On macOS, the VS Code debugger uses LLDB. To configure it:

1. Create a `.vscode/launch.json` file
2. Set up a configuration for C++ debugging with LLDB

### On Linux
For Linux, you'd typically use GDB as the backend for debugging in VS Code.

## Curl

Here are several examples of using curl to test various HTTP functionality. Run your webserver on `localhost:4244` before trying these commands. The `-v` flag provides verbose output, showing both request and response headers.

### 1. POST with multipart/form-data

Create a test file:
```bash
echo -e "This is the content of the file.\nSecond line." > report.txt 
```

Send a multipart POST request:
```bash
curl -v -X POST \
     -F "text_field=Simple text value" \
     -F "file_upload=@report.txt;type=text/plain" \
     http://localhost:4244/cgi/hello.py
```

Clean up:
```bash
rm report.txt
```

**Key parameters explained:**
* `-X POST`: Specifies the POST method (though `-F` implies POST)
* `-F "field=value"`: Sends a simple form field
* `-F "name=@filename;type=mimetype"`: Uploads a file with the specified MIME type
* curl automatically generates the appropriate `Content-Type` header with boundary

### 2. POST with Chunked Transfer Encoding

```bash
# Pipe data to curl using echo
echo -n -e "This is the first chunk.\nAnd this is the second chunk.\nFinally, the third chunk." | \
curl -v -X POST \
     -H "Content-Type: text/plain" \
     -H "Transfer-Encoding: chunked" \
     --data-binary @- \
     http://localhost:4244/cgi/hello.py
```

**Key parameters explained:**
* `-X POST`: Specifies the POST method
* `-H`: Sets specific headers for the request
* `--data-binary @-`: Reads data from standard input and sends it binary-exact
* The combination with `Transfer-Encoding: chunked` makes curl format the data in chunks

### 3. DELETE Request

```bash
curl -v -X DELETE http://localhost:4244/cgi/hello.py/resource/to/delete
```

**Key parameters explained:**
* `-X DELETE`: Specifies the DELETE HTTP method
* The path info `/resource/to/delete` is part of the URL that can be processed by the server

