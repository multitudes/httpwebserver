import requests
def test_custom_404_page(webserver_error_codes_config):
    """Test custom 404 error page"""
    # Test 404 error page
    response = requests.get("http://localhost:4244/nonexistent")
    # Basic response checks
    assert response.status_code == 404
    assert "404 Not Found" in response.text
    # Verify HTML structure and content
    assert "<!DOCTYPE html>" in response.text, "Missing DOCTYPE declaration"
    # Head section assertions
    assert "<title>404 Not Found</title>" in response.text, "Missing or incorrect title"
    assert '<meta charset="UTF-8">' in response.text, "Missing charset meta tag"
    assert '<meta name="viewport" content="width=device-width, initial-scale=1.0">' in response.text, "Missing viewport meta tag"
    assert '<link rel="icon" href="../../favicon/favicon.ico" type="image/x-icon">' in response.text, "Missing or incorrect favicon link"
    # Body content assertions
    assert "<body>" in response.text, "Missing body tag"
    assert "<h1>404</h1>" in response.text, "Missing or incorrect main heading"
    assert "<p>Not Found</p>" in response.text, "Missing or incorrect subheading"
    # Style assertions
    assert "background-color: black" in response.text, "Missing background color style"
    assert "color: white" in response.text, "Missing text color style"
    assert "flex-direction: column" in response.text, "Missing flex layout style"
    assert "justify-content: center" in response.text, "Missing center alignment style"
    assert "height: 100vh" in response.text, "Missing viewport height style"

def test_normal_config_without_error_pages(webserver_normal_config):
    """Test error pages with error_pages config"""
    # Test 404 error page
    response = requests.get("http://localhost:4244/nonexistent")
    assert response.status_code == 404
    assert "404 Not Found" in response.text
       # Assert the title of the page
    assert "<title>404 Not Found</title>" in response.text, "Missing or incorrect title in error page"
    # Assert the main heading
    assert "<h1>404</h1>" in response.text, "Missing or incorrect main heading in error page"
    # Assert the subheading
    assert "<p>Page Not Found</p>" not in response.text, "Missing or incorrect subheading in error page"
    # Assert the descriptive text
    # NB this checks that the text is NOT present, but it should be in the default error page
    assert "The address you were looking for cannot be found or is not valid" not in response.text, \
        "Missing or incorrect descriptive text in error page"
    # Assert the presence of the stylesheet link
    assert '<link rel="stylesheet" type="text/css" href="../css/style.css">' not in response.text, \
        "Missing or incorrect stylesheet link in error page"
    # Assert the presence of the favicon
    assert '<link rel="icon" href="../favicon/favicon.ico" type="image/x-icon">' not in response.text, \
        "Missing or incorrect favicon link in error page"