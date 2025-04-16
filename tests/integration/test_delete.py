import requests

def test_normal_config_delete(webserver_normal_config):
    """Test file deletion (only works with normal config)"""
    delete_url = 'http://localhost:4244/do_not_delete/important.txt' # must be the same as upload_url above
    response = requests.delete(delete_url)
    assert response.status_code == 405
