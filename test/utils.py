import subprocess
import time
import os


def run_server(port):
    env = os.environ.copy()
    env['SSLKEYLOGFILE'] = '/tmp/tls-keys.log'
    return subprocess.Popen([
        "./ion-server/cmake-build-debug/ion-server",
        str(port)
    ], env=env)


def wait_for_port(port, timeout=5):
    print("waiting for port to be open")
    start = time.time()
    while time.time() - start < timeout:
        try:
            result = subprocess.run(
                ["netstat", "-an"],
                capture_output=True,
                text=True,
                timeout=2
            )
            for line in result.stdout.splitlines():
                if ((f".{port}" in line or f"0.0.0.0:{port}" in line) and "LISTEN" in line):
                    print("port is open")
                    return True
        except (subprocess.TimeoutExpired, subprocess.SubprocessError):
            pass
        time.sleep(0.05)
    print("port is closed")
    return False


def curl_http2(url):
    result = subprocess.run(
        ["curl", "--http2", "--insecure", "--verbose", url],
        capture_output=True,
        text=True,
        timeout=5
    )
    print(result.stdout)
    print(result.stderr)
    return result


def assert_curl_response_ok(result):
    assert "< HTTP/2 200 \n< \n" in result.stderr
