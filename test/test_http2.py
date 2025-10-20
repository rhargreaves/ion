import subprocess
import os
from utils import wait_for_port

SERVER_PORT = 8443


def test_http2_tls_connect():
    env = os.environ.copy()
    env['SSLKEYLOGFILE'] = '/tmp/tls-keys.log'
    server = subprocess.Popen(
        ["./ion-server/cmake-build-debug/ion-server",
         str(SERVER_PORT)], env=env)
    assert wait_for_port(SERVER_PORT)
    try:
        result = subprocess.run(
            ["curl", "--http2", "--insecure", "--verbose",
             'https://localhost:' + str(SERVER_PORT)],
            capture_output=True,
            text=True,
            timeout=5,
            env=env
        )
        print(result.stdout)
        print(result.stderr)

        assert result.returncode == 0
        assert "< HTTP/2 200 \n< \n" in result.stderr
    finally:
        server.terminate()
        server.wait()
