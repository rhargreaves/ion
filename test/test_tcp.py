import subprocess
import socket
import time
import os

def test_tcp():
    server = subprocess.Popen(["./ion-server/cmake-build-debug/ion-server", "8080"])
    time.sleep(1)

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 8080))
        data = sock.recv(1024).decode('utf-8')
        assert data == "Hello TCP World\n"
        sock.close()
    finally:
        server.terminate()
        server.wait()

