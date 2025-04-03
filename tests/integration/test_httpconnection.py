import pytest
import requests


WEBSERVER_URL = "http://localhost:4244"

# def test_cgi_basic_health():
#     response = requests.get(f"{WEBSERVER_URL}/cgi-bin/health")
#     assert response.status_code == 200
    
def test_response_basic_health():
	response = requests.get(f"{WEBSERVER_URL}/index.html")
	assert response.status_code == 200

def test_response_404():
	response = requests.get(f"{WEBSERVER_URL}/notfound")
	assert response.status_code == 404
      
# def test_upload_basic_health():
# 	response = requests.get(f"{WEBSERVER_URL}/upload")
# 	assert response.status_code == 200

# def test_upload_file():
# 	files = {'file': ('test.txt', 'This is a test file.')}
# 	response = requests.post(f"{WEBSERVER_URL}/upload", files=files)
# 	assert response.status_code == 200
# 	assert "File uploaded successfully" in response.text

def test_server_responses(webserver, base_url):
	"""Test basic server responses on different addresses/ports"""
	endpoints = [
	"http://0.0.0.0:4244",
	"http://127.0.0.1:4244",
	"http://localhost:4244",
	"http://127.0.0.1:4244/"
	]
	for endpoint in endpoints:
		response = requests.get(endpoint)
		assert response.status_code == 200

# List of all ports from your config
PORTS = [4244, 4245, 4246, 4247, 4248]

# Basic tests for all ports
def test_all_ports_respond():
    for port in PORTS:
        try:
            response = requests.get(f"http://localhost:{port}", timeout=2)
            assert response.status_code == 200
        except requests.ConnectionError:
            pytest.fail(f"Port {port} not responding")

# Test specific features on ports that have them
def test_redirects():
    response = requests.get("http://localhost:4244/42", allow_redirects=False)
    assert response.status_code == 301
    assert "42berlin.de" in response.headers['Location']

def test_autoindex():
    response = requests.get("http://localhost:4244/43")
    assert response.status_code == 200
    assert "Directory Listing" in response.text  # or check for specific files

def test_uploads():
    test_file = {'file': ('test.txt', 'test content')}
    response = requests.post("http://localhost:4244/uploads/", files=test_file)
    assert response.status_code in [200, 201]

# Run with: pytest test_server.py -v