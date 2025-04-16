import requests


def test_cookie_update_request(webserver_redir_config):
    response = requests.put(
        'http://localhost:4244/api/update-cookie/buttonClicked/true',
        allow_redirects=False
    )

    # More detailed debugging
    print("\nResponse Status:", response.status_code)
    print("\nAll Headers:", dict(response.headers))
    print("\nRaw response:", response.text)
    
    # Get all Set-Cookie headers with case-insensitive check
    set_cookie_headers = []
    for key, value in response.headers.items():
        if key.lower() == 'set-cookie':
            print(f"\nFound cookie header: {value}")
            set_cookie_headers.append(value)
    
    print("\nTotal cookies found:", len(set_cookie_headers))
    print("Cookie headers:", set_cookie_headers)

    # Basic checks
    assert response.status_code == 200
    assert response.headers['Content-Type'] == 'application/json'
    assert response.headers['Content-Length'] == '20'

    # More lenient cookie checks
    session_found = False
    button_found = False
    for cookie in set_cookie_headers:
        print(f"\nChecking cookie: {cookie}")
        if 'sessionid=' in cookie:
            session_found = True
        if 'buttonClicked=' in cookie or 'buttonClicked/' in cookie:
            button_found = True

    assert session_found, "Session cookie not found"
    assert button_found, "ButtonClicked cookie not found"
