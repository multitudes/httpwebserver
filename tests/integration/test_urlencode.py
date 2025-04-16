import requests
import subprocess
import pytest

# test for urlencode

def test_urlencode(webserver_normal_config):
    """Test URL encoding"""
# request a page with a space in the URL
    url = 'http://localhost:4244/name with space.html'
    response = requests.get(url)
    assert response.status_code == 200
    assert b'name with space' in response.content

#request a page with a space in the URL and a query string
    url = 'http://localhost:4244/name with space.html?name with=space'
    response = requests.get(url)
    assert response.status_code == 200
    assert b'name with space' in response.content

