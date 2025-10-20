import subprocess
import os
from utils import wait_for_port, curl_http2, assert_curl_response_ok

SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"


def run_server():
    env = os.environ.copy()
    env['SSLKEYLOGFILE'] = '/tmp/tls-keys.log'
    return subprocess.Popen([
        "./ion-server/cmake-build-debug/ion-server",
        str(SERVER_PORT)
    ], env=env)


def test_http2_tls_connect():
    server = run_server()
    assert wait_for_port(SERVER_PORT)
    try:
        result = curl_http2(URL)

        assert result.returncode == 0
        assert_curl_response_ok(result)
    finally:
        server.terminate()
        server.wait()
