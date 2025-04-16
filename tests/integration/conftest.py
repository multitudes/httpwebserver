import subprocess
import time
import pytest
import requests

@pytest.fixture(scope="function")
def webserver_normal_config():
    server = subprocess.Popen(["./webserv", "tests/config/test.conf"])
    time.sleep(0.2)
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config():
    server = subprocess.Popen(["./webserv", "tests/config/empty.conf"])
    time.sleep(0.2)
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config2():
    server = subprocess.Popen(["./webserv", "tests/config/empty2.conf"])
    time.sleep(0.2)
    yield
    server.terminate()
    
@pytest.fixture(scope="function")
def webserver_empty_config3():
    server = subprocess.Popen(["./webserv", "tests/config/empty3.conf"])
    time.sleep(0.2)
    yield
    server.terminate()
    
@pytest.fixture(scope="function")
def webserver_empty_config4():
    server = subprocess.Popen(["./webserv", "tests/config/empty4.conf"])
    time.sleep(0.2)
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_redir_config():
    server = subprocess.Popen(["./webserv", "tests/config/redirections.conf"])
    time.sleep(0.2)
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_error_codes_config():
    server = subprocess.Popen(["./webserv", "tests/config/error_pages.conf"])
    time.sleep(0.2)
    yield
    server.terminate()