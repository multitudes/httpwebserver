import pytest
import requests

# List of all ports from your config
PORTS = [4244, 4245, 4246, 4247, 4248]

WEBSERVER_URL = "http://localhost:4244"

def test_server_responses(webserver, base_url):
    """Test basic server responses"""
    endpoints = [
        base_url.replace("localhost", "0.0.0.0"),
        base_url.replace("localhost", "127.0.0.1"),
        base_url,
        base_url.replace("localhost", "0") + "/"
    ]
    for endpoint in endpoints:
        response = requests.get(endpoint)
        assert response.status_code == 200
    
def test_response_index():
	response = requests.get(f"{WEBSERVER_URL}/index.html")
	assert response.status_code == 200

def test_response_404():
	response = requests.get(f"{WEBSERVER_URL}/notfound")
	assert response.status_code == 404
      
# Basic tests for all ports
def test_all_ports_respond():
    for port in PORTS:
        try:
            response = requests.get(f"http://localhost:{port}", timeout=2)
            assert response.status_code == 200
        except requests.ConnectionError:
            pytest.fail(f"Port {port} not responding")

# Test specific features on ports that have them
# def test_redirects():
#     response = requests.get("http://localhost:4244/42/", allow_redirects=False)
#     assert response.status_code == 301
#     assert "42berlin.de" in response.headers['Location']

def test_internal_redirects():
    response = requests.get("http://localhost:4244/43")
    assert response.status_code == 200
    assert "Hello WWW2" in response.text  # or check for specific files

# def test_uploads():
#     test_file = {'file': ('tests/uploadtest.txt', 'test content')}
#     response = requests.post("http://localhost:4244/uploads/", files=test_file)
#     assert response.status_code in [200, 201]

# def test_upload():
#     # Equivalent to: curl -X POST --data-binary @tests/uploadtest.txt http://localhost:4244/upload/test.txt
#     with open('tests/uploadtest.txt', 'rb') as f:
#         response = requests.post(
#             'http://localhost:4244/upload/test.txt',
#             data=f,
#             headers={'Content-Type': 'application/octet-stream'}
#         )
    
#     assert response.status_code in [200, 201], f"Upload failed: {response.text}"
#     print("Upload successful!")
# Run with: pytest test_server.py -v




def test_upload_with_verification():
    # 1. Prepare test file
    test_file = 'tests/uploadtest.txt'
    upload_url = 'http://localhost:4244/upload/test.txt'
    
    # 2. Upload (same as your curl)
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'application/octet-stream'},
			timeout=5
        )
    
    # 3. Check response
    assert response.status_code in [200, 201]
    
    # 4. Verify file exists on server 
    downloaded = requests.get(upload_url)
    with open(test_file, 'rb') as f:
        original_content = f.read()
        assert downloaded.content == original_content, "Uploaded content doesn't match original"
    
    print("Upload and verification successful!")
	
def test_upload_picture_with_verification():
    # 1. Prepare test file
    test_file = 'tests/egyptiancatsuploadtest.jpeg'
    upload_url = 'http://localhost:4244/upload/egyptiancatsuploadtest.jpeg'
    
    # 2. Upload the picture
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'image/jpeg'},
            timeout=5
        )
    
    # 3. Check response
    assert response.status_code in [200, 201], f"Upload failed: {response.text}"
    
    # 4. Verify file exists on server
    downloaded = requests.get(upload_url)
    with open(test_file, 'rb') as f:
        original_content = f.read()
        assert downloaded.content == original_content, "Uploaded content doesn't match original"
    
    print("Picture upload and verification successful!")


# def test_upload_file():
#     # Path to the test file
#     test_file_path = 'tests/uploadtest.txt'
    
#     # Open the file in binary mode
#     with open(test_file_path, 'rb') as f:
#         files = {'file': (test_file_path, f)}
#         response = requests.post(f"{WEBSERVER_URL}/upload", files=files)
    
#     # Check the response
#     assert response.status_code == 200
#     assert "File uploaded successfully" in response.text

