import time
import os
import re
import asyncio

STATIC_DIR = os.path.join(os.path.dirname(__file__), "../static")
TLS_CERT_PATH = os.path.join(os.path.dirname(__file__), "../../../cert.pem")
TLS_KEY_PATH = os.path.join(os.path.dirname(__file__), "../../../key.pem")

DEFAULT_ARGS = ["-s", "/static", STATIC_DIR,
                "-l", "trace",
                "--under-test",
                "--tls-cert-path", TLS_CERT_PATH,
                "--tls-key-path", TLS_KEY_PATH]

SERVER_PORT = 8443
SERVER_H2C_PORT = 8080

BASE_URL = f"https://localhost:{SERVER_PORT}"
OK_URL = BASE_URL + "/_tests/ok"
SECURITY_URL = BASE_URL + "/_tests/no_new_privs"


async def wait_for_port(port, timeout=5):
    print(f"waiting for port {port} to be open")
    start = time.time()
    while time.time() - start < timeout:
        try:
            _, writer = await asyncio.open_connection('127.0.0.1', port)
            writer.close()
            await writer.wait_closed()
            return True
        except (ConnectionRefusedError, OSError):
            await asyncio.sleep(0.05)
    return False


async def run_server_custom_args(args=None):
    if args is None:
        args = []

    cmd = [os.environ.get('ION_PATH', 'ion-server')] + args
    proc = await asyncio.create_subprocess_exec(
        *cmd,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )
    try:
        stdout, stderr = await asyncio.wait_for(proc.communicate(), timeout=2)
    except asyncio.TimeoutError:
        proc.kill()
        stdout, stderr = await proc.communicate()

    class Result:
        def __init__(self, returncode, stdout, stderr):
            self.returncode = returncode
            self.stdout = stdout.decode()
            self.stderr = stderr.decode()

    return Result(proc.returncode, stdout, stderr)


async def curl(url, args=None):
    if args is None:
        args = []

    cmd = ["curl"] + args + ["--verbose", url]
    proc = await asyncio.create_subprocess_exec(
        *cmd,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE
    )
    stdout, stderr = await proc.communicate()
    print(f"STDERR: {stderr.decode()}")

    class Result:
        def __init__(self, returncode, stdout, stderr):
            self.returncode = returncode
            self.stdout = stdout.decode()
            self.stderr = stderr.decode()

    return Result(proc.returncode, stdout, stderr)


async def curl_http2(url, custom_args=None):
    if custom_args is None:
        custom_args = []

    return await curl(url, custom_args + ["--http2", "--insecure"])


def assert_curl_response_ok(result):
    assert "< HTTP/2 200" in result.stderr


def assert_last_log_line_is_valid(buffer):
    lines = [line.strip() for line in buffer.splitlines() if line.strip()]
    assert lines, "Access log is empty"
    line = lines[0]

    # Combined Log Format Regex:
    # 1. IP
    # 2. Ident & Auth (- -)
    # 3. Timestamp
    # 4. Request
    # 5. Status
    # 6. Size
    # 7. Referrer
    # 8. User Agent
    log_pattern = r'^[\d\.]+ - - \[.*?\] \".*? HTTP/2\" \d{3} (\d+|-) \".*?\" \".*?\"$'

    assert re.match(log_pattern, line), f"Log entry does not match expected format: {line}"
