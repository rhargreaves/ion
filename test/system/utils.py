import subprocess
import time
import os


def run_server(port):
    env = os.environ.copy()
    env['SSLKEYLOGFILE'] = '/tmp/tls-keys.log'
    p = subprocess.Popen([
        os.environ.get('ION_PATH', 'ion-server'),
        "-p",
        str(port)
    ], env=env)
    print(f"server pid = {p.pid}")
    return p


def stop_server(server):
    server.terminate()
    try:
        server.wait(timeout=10)
        assert server.returncode == 0
    except subprocess.TimeoutExpired as e:
        server.kill()
        raise e
    except ProcessLookupError:
        pass


def run_server_help():
    return subprocess.run(
        [
            os.environ.get('ION_PATH', 'ion-server'),
            "--help"
        ],
        capture_output=True,
        text=True,
        timeout=2
    )


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
                    return True
        except (subprocess.TimeoutExpired, subprocess.SubprocessError):
            pass
        time.sleep(0.05)
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
