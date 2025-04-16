import pytest
import requests
import subprocess

def test_custom_error_pages(webserver_normal_config):
    """Test custom error pages"""
    # Test 404 error page
    response = requests.get('http://localhost:4244/404')
    assert response.status_code == 404
    assert "Custom 404 Error Page" in response.text

    # Test 403 error page
    response = requests.get('http://localhost:4244/403')
    assert response.status_code == 403
    assert "Custom 403 Error Page" in response.text

    # Test 500 error page
    response = requests.get('http://localhost:4244/500')
    assert response.status_code == 500
    assert "Custom 500 Error Page" in response.text