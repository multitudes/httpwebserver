import requests
import pytest

def test_cgi_endlessloop(webserver_normal_config):
    url = 'http://localhost:4244/cgi/endlessloop.py'  
    try: 
        response = requests.get(url, timeout=2)
    except Exception as e:
        pytest.fail(f"An unexpected error occurred: {e}")
    assert response.status_code == 504, f"Unexpected status code: {response.status_code}"