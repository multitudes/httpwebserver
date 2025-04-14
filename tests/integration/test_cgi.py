import pytest
import requests
import os



# Test for the CGI script
def test_cgi_script(webserver_normal_config):
    """Test the CGI script"""
    url = 'http://localhost:4244/cgi/hello.py/lol/lol/lol?a=a;b=bf'
    response = requests.get(url)
    # Validate the response
    assert response.status_code == 200, f"Unexpected status code: {response.status_code}"
    assert response.headers["Content-Type"] == "text/html", "Content-Type header mismatch"
    assert "Hello, CGI-World!" in response.text, "Missing expected content in response body"
    assert "<li>CONTENT_TYPE: N/A</li>" in response.text, "CONTENT_TYPE mismatch"
    assert "<li>CONTENT_LENGTH: 0</li>" in response.text, "CONTENT_LENGTH mismatch"
    assert "<li>PATH_INFO: /lol/lol/lol</li>" in response.text, "PATH_INFO mismatch"
    assert "<li>PATH_TRANSLATED: htmltest/www1//lol/lol/lol</li>" in response.text, "PATH_TRANSLATED mismatch"
    assert "<li>SCRIPT_NAME: /cgi-bin/hello.py</li>" in response.text, "SCRIPT_NAME mismatch"
    assert "<li>SERVER_PROTOCOL: HTTP/1.1</li>" in response.text, "SERVER_PROTOCOL mismatch"
    assert "<li>REQUEST_METHOD: GET</li>" in response.text, "REQUEST_METHOD mismatch"
    assert "<li>QUERY_STRING: a=a;b=bf</li>" in response.text, "QUERY_STRING mismatch"
    assert "<li>SERVER_SOFTWARE: VibeServer/1.0</li>" in response.text, "SERVER_SOFTWARE mismatch"
    assert "<li>SERVER_NAME: localhost</li>" in response.text, "SERVER_NAME mismatch"
    assert "<li>SERVER_PORT: 4244</li>" in response.text, "SERVER_PORT mismatch"
    assert "<li>REMOTE_ADDR: 127.0.0.1</li>" in response.text, "REMOTE_ADDR mismatch"
    assert "<li>REMOTE_HOST: localhost</li>" in response.text, "REMOTE_HOST mismatch"
    assert "<li>REMOTE_USER: N/A</li>" in response.text, "REMOTE_USER mismatch"
    assert "<li>GATEWAY_INTERFACE: CGI/1.1</li>" in response.text, "GATEWAY_INTERFACE mismatch"
    assert "<li>AUTH_TYPE: N/A</li>" in response.text, "AUTH_TYPE mismatch"

print("All tests passed!")