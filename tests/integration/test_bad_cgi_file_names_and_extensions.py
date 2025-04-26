import requests

def test_bad_cgi_filename(webserver_normal_config):
    """Test non-existent CGI file returns 404"""
    response = requests.get("http://localhost:4244/cgi/uplooadfile.py")
    assert response.status_code == 404
    assert "404" in response.text

def test_disallowed_cgi_file_extension(webserver_normal_config):
    """Test disallowed extension returns 403 without checking if file exists"""
    response = requests.get("http://localhost:4244/cgi/uploadfile.php")
    assert response.status_code == 403
    assert "403" in response.text

def test_non_executable_cgi_script(webserver_normal_config):
    """Test non-executable script returns 403"""
    response = requests.get("http://localhost:4244/cgi/not_executable.py")
    assert response.status_code == 403
    assert "403" in response.text
