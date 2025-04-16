import requests

def test_normal_config_with_error_pages(webserver_error_codes_config):
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
    # assert "<p>Page Not Found</p>" in response.text, "Missing or incorrect subheading in error page"
    # # Assert the descriptive text
    # assert "The address you were looking for cannot be found or is not valid" in response.text, \
    #     "Missing or incorrect descriptive text in error page"
    # # Assert the presence of the stylesheet link
    # assert '<link rel="stylesheet" type="text/css" href="../css/style.css">' in response.text, \
    #     "Missing or incorrect stylesheet link in error page"
    # # Assert the presence of the favicon
    # assert '<link rel="icon" href="../favicon/favicon.ico" type="image/x-icon">' in response.text, \
    #     "Missing or incorrect favicon link in error page"


def test_normal_config_without_error_pages(webserver_error_codes_config):
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
    assert "The address you were looking for cannot be found or is not valid" not in response.text, \
        "Missing or incorrect descriptive text in error page"
    # Assert the presence of the stylesheet link
    assert '<link rel="stylesheet" type="text/css" href="../css/style.css">' not in response.text, \
        "Missing or incorrect stylesheet link in error page"
    # Assert the presence of the favicon
    assert '<link rel="icon" href="../favicon/favicon.ico" type="image/x-icon">' not in response.text, \
        "Missing or incorrect favicon link in error page"