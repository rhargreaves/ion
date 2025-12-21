import subprocess
import time
import os
import re

ACCESS_LOG_PATH = "/tmp/ion-system-tests-access.log"


def run_server(port, extra_args=None):
    if extra_args is None:
        extra_args = []

    cmd = [os.environ.get('ION_PATH', 'ion-server'),
           "-p",
           str(port),
           "-s",
           "/static",
           os.path.join(os.path.dirname(__file__), "static"),
           ] + extra_args

    p = subprocess.Popen(cmd)
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


def run_server_custom_args(args=None):
    if args is None:
        args = []

    return subprocess.run(
        [
            os.environ.get('ION_PATH', 'ion-server')
        ] + args,
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


def curl(url, args=None):
    if args is None:
        args = []
    args = ["curl"] + args + ["--verbose", url]
    result = subprocess.run(
        args,
        capture_output=True,
        text=True,
        timeout=5
    )
    print(result.stdout)
    print(result.stderr)
    return result


def curl_http2(url, custom_args=None):
    if custom_args is None:
        custom_args = []

    return curl(url, custom_args + ["--http2", "--insecure"])


def assert_curl_response_ok(result):
    assert "< HTTP/2 200 \n" in result.stderr


def assert_last_log_line_is_valid(log_path):
    with open(log_path, 'r') as f:
        lines = [line.strip() for line in f.readlines() if line.strip()]
        assert lines, "Access log is empty"
        last_line = lines[-1]

    # Combined Log Format Regex:
    # 1. IP
    # 2. Ident & Auth (- -)
    # 3. Timestamp
    # 4. Request
    # 5. Statu
    # 6. Size
    # 7. Referrer
    # 8. User Agent
    log_pattern = r'^[\d\.]+ - - \[.*?\] \".*? HTTP/2\" \d{3} (\d+|-) \".*?\" \".*?\"$'

    assert re.match(log_pattern, last_line), f"Log entry does not match expected format: {last_line}"
