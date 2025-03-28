import pytest
import requests


WEBSERVER_URL = "http://localhost:4244"

# def test_cgi_basic_health():
#     response = requests.get(f"{WEBSERVER_URL}/cgi-bin/health")
#     assert response.status_code == 200
    
# def test_response_basic_health():
# 	response = requests.get(f"{WEBSERVER_URL}/index.html")
# 	assert response.status_code == 200

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

# def test_server_responses(webserver, base_url):
# 	"""Test basic server responses on different addresses/ports"""
# 	endpoints = [
# 	"http://0.0.0.0:4244",
# 	"http://127.0.0.1:4244",
# 	"http://localhost:4244",
# 	"http://127.0.0.1:4244/"
# 	]
# 	for endpoint in endpoints:
# 		response = requests.get(endpoint)
# 		assert response.status_code == 200