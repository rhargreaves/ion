import subprocess
import time
import os
import re
import asyncio
import io


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


class ServerRunner:
    def __init__(self, port, extra_args=None, wait_for_port=True, with_defaults=True):
        self.port = port
        self.extra_args = extra_args or []
        self._wait_for_port = wait_for_port
        self._with_defaults = with_defaults
        self.proc = None
        self._stdout_buffer = io.StringIO()
        self._stderr_buffer = io.StringIO()
        self._tasks = []

    async def start(self):
        cmd = [os.environ.get('ION_PATH', 'ion-server'),
               "-p", str(self.port)
               ]

        if self._with_defaults:
            static_dir = os.path.join(os.path.dirname(__file__), "static")
            cert_path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), "cert.pem")
            key_path = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))), "key.pem")
            cmd += ["-s", "/static", static_dir,
                    "-l", "info",
                    "--under-test",
                    "--tls-cert-path", cert_path,
                    "--tls-key-path", key_path]

        cmd += self.extra_args

        self.proc = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        print(f"server pid = {self.proc.pid}")

        self._tasks.append(asyncio.create_task(self._read_stream(self.proc.stdout, self._stdout_buffer)))
        self._tasks.append(asyncio.create_task(self._read_stream(self.proc.stderr, self._stderr_buffer)))

        if self._wait_for_port:
            if not await wait_for_port(self.port):
                await self.stop()
                raise TimeoutError(f"Port {self.port} did not open in time")
        return self

    async def _read_stream(self, stream, buffer):
        try:
            while True:
                line = await stream.readline()
                if not line:
                    break
                buffer.write(line.decode())
        except asyncio.CancelledError:
            pass

    async def stop(self):
        if self.proc:
            if self.proc.returncode is None:
                try:
                    self.proc.terminate()
                    await asyncio.wait_for(self.proc.wait(), timeout=5)
                except (asyncio.TimeoutError, ProcessLookupError):
                    try:
                        self.proc.kill()
                        await self.proc.wait()
                    except ProcessLookupError:
                        pass

            for task in self._tasks:
                if not task.done():
                    task.cancel()
            if self._tasks:
                await asyncio.gather(*self._tasks, return_exceptions=True)
            self._tasks = []

    def get_stdout(self):
        return self._stdout_buffer.getvalue()

    def get_stderr(self):
        return self._stderr_buffer.getvalue()

    async def __aenter__(self):
        return await self.start()

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self.stop()


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
    print(f"STDOUT: {stdout.decode()}")
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
    print("stdout: " + result.stdout)
    print("stderr: " + result.stderr)
    assert "< HTTP/2 200" in result.stderr


def assert_last_log_line_is_valid(buffer):
    lines = [line.strip() for line in buffer.splitlines() if line.strip()]
    assert lines, "Access log is empty"
    last_line = lines[-1]

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

    assert re.match(log_pattern, last_line), f"Log entry does not match expected format: {last_line}"
