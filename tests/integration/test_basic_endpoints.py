import pytest
import requests

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


def test_normal_config_responses_for_each_port(webserver_normal_config):
    """Test endpoints on all ports defined in the config file"""
    endpoint = "http://localhost:4244"
    response = requests.get(endpoint)
    assert response.status_code == 200
    assert "<h1>Hello Website</h1>" in response.text
    assert "Webserv 200 OK" in response.text

    endpoint = "http://localhost:4245"
    response = requests.get(endpoint)
    assert response.status_code == 200
    assert "<h1>Hello Website</h1>" in response.text
    assert "Webserv 200 OK" in response.text

    endpoint = "http://localhost:4246"
    response = requests.get(endpoint)
    assert response.status_code == 200
    assert "<h1>Hello WWW2 index.html</h1>" in response.text
    assert "Webserv 200 OK" in response.text

    endpoint = "http://localhost:4247"
    response = requests.get(endpoint)
    assert response.status_code == 200
    assert "<h1>Hello WWW3 index.html</h1>" in response.text
    assert "Webserv 200 OK" in response.text

    endpoint = "http://localhost:4248"
    response = requests.get(endpoint)
    assert response.status_code == 200
    assert "<h1>Hello WWW3 index.html</h1>" in response.text
    assert "Webserv 200 OK" in response.text

