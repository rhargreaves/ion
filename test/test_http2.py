import subprocess
import time

def test_http2_tls_connect():
    server = subprocess.Popen(["./ion-server/cmake-build-debug/ion-server", "8443"])
    time.sleep(1)

    try:
        result = subprocess.run(
            ["curl", "--http2", "--insecure", "https://localhost:8443"],
            capture_output=True,
            text=True,
            timeout=5
        )
        print(result.stdout)
        print(result.stderr)
        assert result.returncode == 0
        assert "Hello" in result.stdout
    finally:
        server.terminate()
        server.wait()

