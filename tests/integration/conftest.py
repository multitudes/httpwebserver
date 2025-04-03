import subprocess
import time
import pytest
import requests

# Single fixture to start/stop server (just like you have)
@pytest.fixture(scope="session", autouse=True)
def webserver():
    server = subprocess.Popen(["./webserv", "config/test.conf"])
    time.sleep(1)  # Give server time to start (all ports)
    yield
    server.terminate()
    server.wait()

@pytest.fixture
def base_url():
    return "http://localhost:4244"  # Default port
