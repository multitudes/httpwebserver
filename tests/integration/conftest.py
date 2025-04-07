import subprocess
import time
import pytest
import requests

# # Single fixture to start/stop server (just like you have)
# @pytest.fixture(scope="session", autouse=True)
# def webserver():
#     server = subprocess.Popen(["./webserv", "config/test.conf"])
#     time.sleep(1)  # Give server time to start (all ports)
#     yield
#     server.terminate()
#     server.wait()

# @pytest.fixture
# def base_url():
#     return "http://localhost:4244"  # Default port


@pytest.fixture(scope="function")
def webserver_normal_config():
    server = subprocess.Popen(["./webserv", "tests/config/test.conf"])
    time.sleep(1)
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config():
    server = subprocess.Popen(["./webserv", "tests/config/empty.conf"])
    time.sleep(1)
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config2():
    server = subprocess.Popen(["./webserv", "tests/config/empty2.conf"])
    time.sleep(1)
    yield
    server.terminate()
    
@pytest.fixture(scope="function")
def webserver_empty_config3():
    server = subprocess.Popen(["./webserv", "tests/config/empty3.conf"])
    time.sleep(1)
    yield
    server.terminate()
    
@pytest.fixture(scope="function")
def webserver_empty_config4():
    server = subprocess.Popen(["./webserv", "tests/config/empty4.conf"])
    time.sleep(1)
    yield
    server.terminate()


# def test_empty_config(webserver_empty_config):
#     response = requests.get("http://localhost:4244")
#     assert response.status_code == 500
