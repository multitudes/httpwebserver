import pytest
import requests

WEBSERVER_URL = "http://localhost:4244"

def test_backend_health():
    response = requests.get(f"{WEBSERVER_URL}/cgi-bin/health")
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