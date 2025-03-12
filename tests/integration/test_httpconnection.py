import pytest
import requests

WEBSERVER_URL = "http://localhost:4244"

def test_backend_health():
    response = requests.get(f"{WEBSERVER_URL}/metrics")
    assert response.status_code == 200