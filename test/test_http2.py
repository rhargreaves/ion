import subprocess
import time
import os

SERVER_PORT = 8443

def test_http2_tls_connect():
    env = os.environ.copy()
    env['SSLKEYLOGFILE'] = '/tmp/tls-keys.log'
    server = subprocess.Popen(["./ion-server/cmake-build-debug/ion-server",
                               str(SERVER_PORT)], env=env)
    time.sleep(1)
    try:
        result = subprocess.run(["curl", "--http2", "--insecure", "--trace", "trace.log",
                'https://localhost:' + str(SERVER_PORT)],
            capture_output=True,
            text=True,
            timeout=5,
            env=env
        )
        print(result.stdout)
        print(result.stderr)
        assert result.returncode == 0
        assert "Hello" in result.stdout
    finally:
        server.terminate()
        server.wait()

