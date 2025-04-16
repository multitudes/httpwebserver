import requests

def test_redirections1(webserver_redir_config):
    """Test redirections for specific locations"""
    # Test redirection for /42
    response_42 = requests.get("http://localhost:4244/42", allow_redirects=False)
    assert response_42.status_code == 301, f"Expected 301, got {response_42.status_code}"
    assert response_42.headers["Location"] == "http://42berlin.de/", \
        f"Expected Location header 'http://42berlin.de/', got {response_42.headers.get('Location')}"

    
def test_redirections1(webserver_redir_config):
    """Test redirections for specific locations"""

    # Test redirection for /go
    response_go = requests.get("http://localhost:4244/go", allow_redirects=False)
    assert response_go.status_code == 301, f"Expected 301, got {response_go.status_code}"
    assert response_go.headers["Location"] == "/here/index.html", \
        f"Expected Location header '/here/index.html', got {response_go.headers.get('Location')}"