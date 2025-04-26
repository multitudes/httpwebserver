import subprocess
import time
import pytest

def start_webserver(config_path):
    """Helper function to start the webserver with a given config."""
    server = subprocess.Popen(["./webserv", config_path])
    time.sleep(0.01)
    return server

@pytest.fixture(scope="function")
def webserver_normal_config():
    server = start_webserver("tests/config/default.conf")
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config():
    server = start_webserver("tests/config/empty.conf")
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config2():
    server = start_webserver("tests/config/empty2.conf")
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config3():
    server = start_webserver("tests/config/empty3.conf")
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_empty_config4():
    server = start_webserver("tests/config/empty4.conf")
    yield
    server.terminate()

# invalid syntax
@pytest.fixture(scope="function")
def webserver_empty_config5():
    server = start_webserver("tests/config/empty5.conf")
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_redir_config():
    server = start_webserver("tests/config/redirections.conf")
    yield
    server.terminate()

@pytest.fixture(scope="function")
def webserver_error_codes_config():
    server = start_webserver("tests/config/error_pages.conf")
    yield
    server.terminate()