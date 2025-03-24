import subprocess
import time
import pytest

@pytest.fixture(scope="session", autouse=True)
def start_web_server():
    # Start the web server
    server_process = subprocess.Popen(["./webserv"])
    
    # Wait for the server to start
    time.sleep(1)
    
    yield server_process
    
    # Teardown: Stop the web server
    server_process.terminate()
    server_process.wait()