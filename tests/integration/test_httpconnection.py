import pytest
import requests
import os

WEBSERVER_URL = "http://localhost:4244"

def test_backend_health():
    response = requests.get(f"{WEBSERVER_URL}/metrics")
    assert response.status_code == 200
    
def test_file_upload():
    # Path to the file to upload
    file_path = os.path.join(os.path.dirname(__file__), "test.txt")
    
    # Perform the file upload
    with open(file_path, "rb") as f:
        files = {"file": f}
        response = requests.post(f"{WEBSERVER_URL}/test.txt", files=files)
    
    # Assert the upload was successful
    assert response.status_code == 200

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