import pytest
import requests
import subprocess

# Tests for NORMAL config (test.conf)
def test_normal_config_responses(webserver_normal_config):
    """Test endpoints on default port (4244)"""
    endpoints = [
        "http://localhost:4244",
        "http://0.0.0.0:4244",
        "http://127.0.0.1:4244",
    ]
    for endpoint in endpoints:
        response = requests.get(endpoint)
        assert response.status_code == 200

def test_normal_config_ports(webserver_normal_config):
    """Test all ports defined in PORTS"""
    PORTS = [4244, 4245, 4246, 4247, 4248]
    for port in PORTS:
        try:
            response = requests.get(f"http://localhost:{port}", timeout=2)
            assert response.status_code == 200
        except requests.ConnectionError:
            pytest.fail(f"Port {port} not responding")

def test_normal_config_upload(webserver_normal_config):
    """Test file upload (only works with normal config)"""
    test_file = 'tests/uploadtest.txt'
    upload_url = 'http://localhost:4244/upload/test.txt'
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'text/plain'},
            timeout=1
        )
    assert response.status_code in [200, 201]
    downloaded = requests.get(upload_url)
    with open(test_file, 'rb') as f:
        assert downloaded.content == f.read()

# Tests for EMPTY config (empty.conf)
def test_empty_config_startup():
    """Test that the server fails to start with an empty config"""
    result = subprocess.run(
        ["./webserv", "config/empty.conf"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    # Assert that the exit code is non-zero (indicating an error)
    assert result.returncode != 0, f"Server started with empty config: {result.stdout.decode()}\n{result.stderr.decode()}"