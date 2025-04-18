import requests
from urllib.parse import quote

# test for UPLOAD and DELETE, first in default uploads, then in a folder with return directive
#first 2 tests are for the default upload folder
def test_normal_config_upload(webserver_normal_config):
    """Test file upload (only works with normal config)"""
    test_file = 'tests/uploadtest.txt'
    upload_url = 'http://localhost:4244/upload/test.txt'
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'text/plain'},
            timeout=1
        )
    assert response.status_code in [200, 201]
    downloaded = requests.get(upload_url)
    with open(test_file, 'rb') as f:
        assert downloaded.content == f.read()

#check if  DELETE request can delete the file from previous test
def test_normal_config_delete(webserver_normal_config):
    """Test file deletion (only works with normal config)"""
    delete_url = 'http://localhost:4244/upload/test.txt' # must be the same as upload_url above
    response = requests.delete(delete_url)
    assert response.status_code == 200
    # Check if the file is deleted
    response = requests.get(delete_url)
    assert response.status_code == 404

# Test for upload and delete in a folder with return directive
def test_redirect_upload(webserver_normal_config):
    """Test file upload (only works with normal config)"""
    test_file = 'tests/uploadtest.txt'
    upload_url = 'http://localhost:4244/43/test.txt'
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'text/plain'},
            timeout=1
        )
    assert response.status_code in [200, 201]
    downloaded = requests.get(upload_url)
    with open(test_file, 'rb') as f:
        assert downloaded.content == f.read()

#check if  DELETE request can delete the file from previous test
def test_redirect_delete(webserver_normal_config):
    """Test file deletion (only works with normal config)"""
    delete_url = 'http://localhost:4244/43/test.txt' # must be the same as upload_url above
    response = requests.delete(delete_url)
    assert response.status_code == 200
    # Check if the file is deleted
    response = requests.get(delete_url)
    assert response.status_code == 404
# end of UPLOAD and DELETE tests
        
def test_normal_config_upload2(webserver_normal_config):
    """Test file upload with egyptiancatsuploadtest.jpeg (only works with normal config)"""
    test_file = 'tests/egyptiancatsuploadtest.jpeg'
    upload_url = 'http://localhost:4244/upload/egyptiancatsuploadtest.jpeg'

    # Upload the file
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'image/jpeg'},  # Set appropriate content type
            timeout=1
        )
    assert response.status_code in [200, 201], f"Upload failed with status code {response.status_code}"

    # Download the file and verify its content
    downloaded = requests.get(upload_url)
    assert downloaded.status_code == 200, f"Download failed with status code {downloaded.status_code}"
    with open(test_file, 'rb') as f:
        assert downloaded.content == f.read(), "Uploaded content does not match the original file"
        

#check if  DELETE request can delete the file from previous test
def test_normal_config_delete2(webserver_normal_config):
    """Test file deletion (only works with normal config)"""
    delete_url = 'http://localhost:4244/upload/egyptiancatsuploadtest.jpeg' # must be the same as upload_url above
    response = requests.delete(delete_url)
    assert response.status_code == 200
    # Check if the file is deleted
    response = requests.get(delete_url)
    assert response.status_code == 404


def test_normal_config_upload_large_file1(webserver_normal_config):
    """Test file upload with screeenshot.jpeg 2 MB """
    test_file = 'tests/screenshot.png'
    upload_url = 'http://localhost:4244/upload/screenshot.png'

    # Upload the file
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'image/png'},  # Set appropriate content type
            timeout=1
        )
    assert response.status_code in [200, 201], f"Upload failed with status code {response.status_code}"

    # Download the file and verify its content
    downloaded = requests.get(upload_url)
    assert downloaded.status_code == 200, f"Download failed with status code {downloaded.status_code}"
    with open(test_file, 'rb') as f:
        assert downloaded.content == f.read(), "Uploaded content does not match the original file"
        
def test_delete_large_file(webserver_normal_config):
    """Test file deletion (only works with normal config)"""
    delete_url = 'http://localhost:4244/upload/screenshot.png' # must be the same as upload_url above
    response = requests.delete(delete_url)
    assert response.status_code == 200
    # Check if the file is deleted
    response = requests.get(delete_url)
    assert response.status_code == 404


def test_normal_config_upload_large_file_with_space(webserver_normal_config):
    """Test file upload with a file name containing spaces"""
    # File with a space in its name
    test_file = 'tests/screenshot + space.png'
    # URL-encode the file name
    encoded_file_name = quote('screenshot + space.png')
    upload_url = f'http://localhost:4244/upload/{encoded_file_name}'

    # Upload the file
    with open(test_file, 'rb') as f:
        response = requests.post(
            upload_url,
            data=f,
            headers={'Content-Type': 'image/png'},  # Set appropriate content type
            timeout=1
        )
    assert response.status_code in [200, 201], f"Upload failed with status code {response.status_code}"

    # Download the file and verify its content
    downloaded = requests.get(upload_url)
    assert downloaded.status_code == 200, f"Download failed with status code {downloaded.status_code}"
    with open(test_file, 'rb') as f:
        assert downloaded.content == f.read(), "Uploaded content does not match the original file"


def test_delete_large_file_with_space(webserver_normal_config):
    """Test file deletion (only works with normal config)"""
    # URL-encode the file name
    encoded_file_name = quote('screenshot + space.png')
    delete_url = f'http://localhost:4244/upload/{encoded_file_name}'  # Must match the upload URL
    response = requests.delete(delete_url)
    assert response.status_code == 200, f"Delete failed with status code {response.status_code}"
    
    # Check if the file is deleted
    response = requests.get(delete_url)
    assert response.status_code == 404, "File was not deleted successfully"