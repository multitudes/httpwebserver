import requests
import pytest 

# Test for the CGI script
def test_cgi_script(webserver_normal_config):
    """Test the CGI script"""
    url = 'http://localhost:4244/cgi/hello.py/lol/lol/lol?a=a;b=bf'
    response = requests.get(url)
    # Validate the response
    assert response.status_code == 200, f"Unexpected status code: {response.status_code}"
    assert response.headers["Content-Type"] == "text/html", "Content-Type header mismatch"
    assert "Hello, CGI-World!" in response.text, "Missing expected content in response body"
    assert "<li>CONTENT_TYPE: </li>" in response.text, "CONTENT_TYPE mismatch"
    assert "<li>CONTENT_LENGTH: </li>" in response.text, "CONTENT_LENGTH mismatch"
    assert "<li>PATH_INFO: /lol/lol/lol</li>" in response.text, "PATH_INFO mismatch"
    assert "<li>PATH_TRANSLATED: htmltest/www1/lol/lol/lol</li>" in response.text, "PATH_TRANSLATED mismatch"
    assert "<li>SCRIPT_NAME: /cgi-bin/hello.py</li>" in response.text, "SCRIPT_NAME mismatch"
    assert "<li>SERVER_PROTOCOL: HTTP/1.1</li>" in response.text, "SERVER_PROTOCOL mismatch"
    assert "<li>REQUEST_METHOD: GET</li>" in response.text, "REQUEST_METHOD mismatch"
    assert "<li>QUERY_STRING: a=a;b=bf</li>" in response.text, "QUERY_STRING mismatch"
    assert "<li>SERVER_SOFTWARE: VibeServer/1.0</li>" in response.text, "SERVER_SOFTWARE mismatch"
    assert "<li>SERVER_NAME: localhost</li>" in response.text, "SERVER_NAME mismatch"
    assert "<li>SERVER_PORT: 4244</li>" in response.text, "SERVER_PORT mismatch"
    assert "<li>REMOTE_ADDR: 127.0.0.1</li>" in response.text, "REMOTE_ADDR mismatch"
    assert "<li>REMOTE_HOST: localhost</li>" in response.text, "REMOTE_HOST mismatch"
    assert "<li>REMOTE_USER: N/A</li>" in response.text, "REMOTE_USER mismatch"
    assert "<li>GATEWAY_INTERFACE: CGI/1.1</li>" in response.text, "GATEWAY_INTERFACE mismatch"
    assert "<li>AUTH_TYPE: N/A</li>" in response.text, "AUTH_TYPE mismatch"

# --- Test for POST with multipart/form-data ---
'''
example curl    
curl -v -X POST \
             -F "text_field=Simple text value" \
             -F "file_upload=@report.txt;type=text/plain" \
             http://localhost:4244/cgi/hello.py

'''
def test_cgi_post_multipart(webserver_normal_config):
    """Test the CGI script with a POST request using multipart/form-data"""
    url = 'http://localhost:4244/cgi/hello.py' # Target CGI script
    
    # Define the multipart data
    # 'files' tells requests to encode as multipart/form-data
    # Format: { 'form_field_name': ('filename', 'file_content', 'content_type') }
    # Or for simple fields: { 'form_field_name': (None, 'field_value') }
    files_data = {
        'text_field': (None, 'Simple text value'),
        'file_upload': ('report.txt', 'This is the content of the file.\nSecond line.', 'text/plain')
    }
    
    # Make the POST request
    response = requests.post(url, files=files_data)
    
    # Validate the response
    assert response.status_code == 200, f"Unexpected status code: {response.status_code}"
    assert response.headers.get("Content-Type", "").startswith("text/html"), "Content-Type header mismatch"
    assert "Hello, CGI-World!" in response.text, "Missing expected content in response body"
    
    # Validate CGI environment variables specific to POST multipart
    assert "<li>REQUEST_METHOD: POST</li>" in response.text, "REQUEST_METHOD mismatch"
    # CONTENT_TYPE will include a dynamic boundary, so check the start
    assert "<li>CONTENT_TYPE: multipart/form-data; boundary=" in response.text, "CONTENT_TYPE mismatch"
    # CONTENT_LENGTH should be present and non-empty for multipart
    assert "<li>CONTENT_LENGTH: </li>" not in response.text, "CONTENT_LENGTH should be set for multipart POST"
    
    # Check other common variables (adjust path/query specific ones if needed)
    assert "<li>PATH_INFO: /</li>" in response.text, "PATH_INFO should be empty"
    assert "<li>QUERY_STRING: </li>" in response.text, "QUERY_STRING should be empty"
    assert "<li>SCRIPT_NAME: /cgi-bin/hello.py</li>" in response.text, "SCRIPT_NAME mismatch" # Adjusted from original test
    assert "<li>SERVER_PROTOCOL: HTTP/1.1</li>" in response.text, "SERVER_PROTOCOL mismatch"
    assert "<li>SERVER_SOFTWARE: VibeServer/1.0</li>" in response.text, "SERVER_SOFTWARE mismatch"
    assert "<li>SERVER_NAME: localhost</li>" in response.text, "SERVER_NAME mismatch"
    assert "<li>SERVER_PORT: 4244</li>" in response.text, "SERVER_PORT mismatch"
    assert "<li>REMOTE_ADDR: 127.0.0.1</li>" in response.text, "REMOTE_ADDR mismatch"
    assert "<li>GATEWAY_INTERFACE: CGI/1.1</li>" in response.text, "GATEWAY_INTERFACE mismatch"
    
    # Validate the raw multipart data for text_field
    assert 'form-data; name="text_field"' in response.text, "Missing 'text_field' in raw multipart data"
   


# --- Test for POST with Chunked Transfer Encoding ---
# Generator function to simulate chunked data
'''
example curl
echo -n -e "This is the first chunk.\nAnd this is the second chunk.\nFinally, the third chunk." | \
    curl -v -X POST \
         -H "Content-Type: text/plain" \
         -H "Transfer-Encoding: chunked" \
         --data-binary @- \
         http://localhost:4244/cgi/hello.py
'''
def chunked_data_generator():
    yield b"This is the first chunk.\n"
    yield b"And this is the second chunk.\n"
    yield b"Finally, the third chunk."

def test_cgi_post_chunked(webserver_normal_config):
    """Test the CGI script with a POST request using chunked transfer encoding"""
    url = 'http://localhost:4244/cgi/hello.py' # Target CGI script
    headers = {
        'Content-Type': 'text/plain',
        'Transfer-Encoding': 'chunked' # requests handles this header when data is an iterator
    }

    # Make the POST request with a generator for the data
    # This tells requests to use chunked encoding
    response = requests.post(url, data=chunked_data_generator(), headers=headers)

    # Validate the response
    assert response.status_code == 200, f"Unexpected status code: {response.status_code}"
    assert response.headers.get("Content-Type", "").startswith("text/html"), "Content-Type header mismatch"
    assert "Hello, CGI-World!" in response.text, "Missing expected content in response body"

    # Validate CGI environment variables specific to chunked POST
    assert "<li>REQUEST_METHOD: POST</li>" in response.text, "REQUEST_METHOD mismatch"
    # CONTENT_LENGTH should NOT be set for chunked requests according to CGI spec
    assert "<li>CONTENT_LENGTH: 80</li>" in response.text, "CONTENT_LENGTH should be set correctly for chunked POST"
    # CONTENT_TYPE should reflect what we sent
    assert "<li>CONTENT_TYPE: text/plain</li>" in response.text, "CONTENT_TYPE mismatch"
   
    # Check other common variables (adjust path/query specific ones if needed)
    assert "<li>PATH_INFO: /</li>" in response.text, "PATH_INFO should be /"
    assert "<li>QUERY_STRING: </li>" in response.text, "QUERY_STRING should be empty"
    assert "<li>SCRIPT_NAME: /cgi-bin/hello.py</li>" in response.text, "SCRIPT_NAME mismatch" # Adjusted
    assert "<li>SERVER_PROTOCOL: HTTP/1.1</li>" in response.text, "SERVER_PROTOCOL mismatch"
    assert "<li>SERVER_SOFTWARE: VibeServer/1.0</li>" in response.text, "SERVER_SOFTWARE mismatch"
    assert "<li>SERVER_NAME: localhost</li>" in response.text, "SERVER_NAME mismatch"
    assert "<li>SERVER_PORT: 4244</li>" in response.text, "SERVER_PORT mismatch"
    assert "<li>GATEWAY_INTERFACE: CGI/1.1</li>" in response.text, "GATEWAY_INTERFACE mismatch"

    # Validate the body content
    assert "<h2>Body Received</h2>" in response.text, "Missing 'Body Received' section in response"
    assert "<p>First 100 characters:</p>" in response.text, "Missing 'First 100 characters' section in response"
    expected_body = """<pre>This is the first chunk.
And this is the second chunk.
Finally, the third chunk.</pre>"""
    assert expected_body in response.text, "Body content mismatch in response"


# --- Test for DELETE request ---
def test_cgi_delete(webserver_normal_config):
    """Test the CGI script with a DELETE request"""
    # Include some path info to simulate deleting a resource
    url = 'http://localhost:4244/cgi/hello.py/resource/to/delete' 
    
    # Make the DELETE request (no body in this case)
    response = requests.delete(url)
    
    # Validate the response
    assert response.status_code == 200, f"Unexpected status code: {response.status_code}" # Or maybe 204 if your script handles it
    assert response.headers.get("Content-Type", "").startswith("text/html"), "Content-Type header mismatch"
    assert "Hello, CGI-World!" in response.text, "Missing expected content in response body"
    
    # Validate CGI environment variables specific to DELETE
    assert "<li>REQUEST_METHOD: DELETE</li>" in response.text, "REQUEST_METHOD mismatch"
    # CONTENT_LENGTH should be empty/unset or 0 if no body is sent
    # Check for empty:
    assert "<li>CONTENT_LENGTH: </li>" in response.text or "<li>CONTENT_LENGTH: 0</li>" in response.text, "CONTENT_LENGTH should be empty or 0 for DELETE with no body"
    # CONTENT_TYPE should be empty/unset if no body is sent
    assert "<li>CONTENT_TYPE: </li>" in response.text, "CONTENT_TYPE should be empty/unset for DELETE with no body"
    
    # Check path/query specific variables
    assert "<li>PATH_INFO: /resource/to/delete</li>" in response.text, "PATH_INFO mismatch"
    # Assuming PATH_TRANSLATED maps similarly to your GET test structure
    # Adjust the base path 'htmltest/www1/' as needed for your setup
    assert "<li>PATH_TRANSLATED: htmltest/www1/resource/to/delete</li>" in response.text, "PATH_TRANSLATED mismatch" 
    assert "<li>QUERY_STRING: </li>" in response.text, "QUERY_STRING should be empty"
    assert "<li>SCRIPT_NAME: /cgi-bin/hello.py</li>" in response.text, "SCRIPT_NAME mismatch" # Adjusted

    # Check other common variables
    assert "<li>SERVER_PROTOCOL: HTTP/1.1</li>" in response.text, "SERVER_PROTOCOL mismatch"
    assert "<li>SERVER_SOFTWARE: VibeServer/1.0</li>" in response.text, "SERVER_SOFTWARE mismatch"
    assert "<li>SERVER_NAME: localhost</li>" in response.text, "SERVER_NAME mismatch"
    assert "<li>SERVER_PORT: 4244</li>" in response.text, "SERVER_PORT mismatch"
    assert "<li>GATEWAY_INTERFACE: CGI/1.1</li>" in response.text, "GATEWAY_INTERFACE mismatch"

# ... existing imports ...

# --- Test for Perl CGI script (hello.pl) ---
def test_cgi_perl_hello(webserver_normal_config):
    """Test the Perl CGI script hello.pl"""
    url = 'http://localhost:4244/cgi/hello.pl'  
    try: 
        response = requests.get(url, timeout=0.1)
    except Exception as e:
        pytest.fail(f"An unexpected error occurred: {e}")

    # Validate the response status and headers
    assert response.status_code == 200, f"Unexpected status code: {response.status_code}"
    # Assuming the Perl script outputs text/html Content-Type header
    assert response.headers.get("Content-Type", "").startswith("text/html"), "Content-Type header mismatch"

    # Validate the HTML structure and content
    assert "<title>Ricken's Quotes</title>" in response.text, "Missing correct title tag"
    assert "<h1>Ricken's Philosophical Quotes</h1>" in response.text, "Missing main heading"
    assert '<link rel="stylesheet" type="text/css" href="/css/style.css">' in response.text, "Missing CSS link"
    assert '<link rel="icon" href="/favicon/favicon.ico" type="image/x-icon">' in response.text, "Missing favicon link"

    # Validate the presence of specific quotes within <div class="quote">
    # Checking a few key quotes for brevity and robustness
    assert '<div class="quote">Bullies are bull and lies <br>---<br></div>' in response.text, "Missing quote: Bullies"
    assert '<div class="quote">In the center of industry is dust.<br>---<br></div>' in response.text, "Missing quote: Industry"
    assert '<div class="quote">A good person will follow the rules. A great person will follow himself.<br>---<br></div>' in response.text, "Missing quote: Good person"
    assert 'Also they are made of metal, whereas man is made of skin.<br>---<br></div>' in response.text, "Missing quote fragment: Metal/Skin"
    assert 'you are a mile away from them and you have their shoes.<br>---<br></div>' in response.text, "Missing quote fragment: Shoes"

