import asyncio
import io
import os
from pathlib import Path

from .utils import wait_for_port

REPO_ROOT = Path(__file__).resolve().parents[3]
WORKER_DIR = REPO_ROOT / "worker"
DEFAULT_CONFIG = WORKER_DIR / "config.test.capnp"
WORKERD_BIN = WORKER_DIR / "node_modules" / ".bin" / "workerd"


class WorkerRunner:
    def __init__(self, port, config=None, extra_args=None, env=None, wait_for_port=True):
        self.port = port
        self.config = Path(config) if config is not None else DEFAULT_CONFIG
        self.extra_args = list(extra_args) if extra_args is not None else []
        self.env = env if env is not None else os.environ.copy()
        self._wait_for_port = wait_for_port
        self.proc = None
        self._stdout_buffer = io.StringIO()
        self._stderr_buffer = io.StringIO()
        self._tasks = []

    def _workerd(self):
        if WORKERD_BIN.exists():
            return str(WORKERD_BIN)
        raise FileNotFoundError(
            f"workerd not found at {WORKERD_BIN}. Run 'npm ci' in {WORKER_DIR}."
        )

    async def start(self):
        if not self.config.exists():
            raise FileNotFoundError(f"Worker config not found: {self.config}")

        cmd = [self._workerd(), "serve", str(self.config), "--experimental"] + self.extra_args

        print(f"Starting worker with cmd: {' '.join(cmd)}, cwd: {WORKER_DIR}")
        self.proc = await asyncio.create_subprocess_exec(
            *cmd,
            cwd=str(WORKER_DIR),
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            env=self.env,
        )
        print(f"worker pid = {self.proc.pid}")

        self._tasks.append(asyncio.create_task(self._read_stream(self.proc.stdout, self._stdout_buffer)))
        self._tasks.append(asyncio.create_task(self._read_stream(self.proc.stderr, self._stderr_buffer)))

        if self._wait_for_port:
            if not await wait_for_port(self.port, 10):
                await self.stop()
                stdout = self.get_stdout()
                stderr = self.get_stderr()
                if self.proc.returncode is not None:
                    raise RuntimeError(
                        f"Worker exited with code {self.proc.returncode}.\nSTDOUT: {stdout}\nSTDERR: {stderr}")
                raise TimeoutError(
                    f"Port {self.port} did not open in time.\nSTDOUT: {stdout}\nSTDERR: {stderr}")
        return self

    async def _read_stream(self, stream, buffer):
        try:
            while not stream.at_eof():
                data = await stream.read(64 * 1024)
                if not data:
                    break
                text = data.decode(errors='replace')
                buffer.write(text)
                print(text, end='', flush=True)
        except asyncio.CancelledError:
            pass

    async def stop(self):
        if self.proc:
            if self.proc.returncode is None:
                try:
                    self.proc.terminate()
                    await asyncio.wait_for(self.proc.wait(), timeout=2)
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
        if exc_type is not None:
            print(f"\n--- Test Failed/Interrupted ({exc_type.__name__}) ---")
            print(f"Worker STDOUT:\n{self.get_stdout()}")
            print(f"Worker STDERR:\n{self.get_stderr()}")

        await self.stop()
