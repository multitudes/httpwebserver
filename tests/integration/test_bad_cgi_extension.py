import requests

def test_normal_config_with_error_pages(webserver_error_codes_config):
    """Test error pages with error_pages config"""
    # Test 404 error page
    response = requests.get("http://localhost:4244/cgi/uploadfile.pylol")
    assert response.status_code == 404
    assert "404 Not Found" in response.text