import os
import asyncio
import io
from .utils import DEFAULT_ARGS, wait_for_port


class ServerRunner:
    def __init__(self, port, args=None, extra_args=None, wait_for_port=True):
        if args is None:
            args = list(DEFAULT_ARGS)
        else:
            args = list(args)

        if extra_args is not None:
            args.extend(extra_args)

        self.args = args
        self.port = port
        self._wait_for_port = wait_for_port
        self.proc = None
        self._stdout_buffer = io.StringIO()
        self._stderr_buffer = io.StringIO()
        self._tasks = []

    async def start(self):
        cmd = [os.environ.get('ION_PATH', 'ion-server'),
               "-p", str(self.port)
               ] + self.args

        print(f"Starting server with cmd: {' '.join(cmd)}")
        self.proc = await asyncio.create_subprocess_exec(
            *cmd,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE
        )

        print(f"server pid = {self.proc.pid}")

        self._tasks.append(asyncio.create_task(self._read_stream(self.proc.stdout, self._stdout_buffer)))
        self._tasks.append(asyncio.create_task(self._read_stream(self.proc.stderr, self._stderr_buffer)))

        # Give it a tiny bit of time to fail if it's going to fail immediately (e.g. port bind error)
        try:
            await asyncio.wait_for(self.proc.wait(), timeout=0.1)
            # If we get here, the process already exited
            stdout = self.get_stdout()
            stderr = self.get_stderr()
            raise RuntimeError(
                f"Server exited immediately with code {self.proc.returncode}.\nSTDOUT: {stdout}\nSTDERR: {stderr}")
        except asyncio.TimeoutError:
            # Process is still running, which is what we want
            pass

        if self._wait_for_port:
            if not await wait_for_port(self.port):
                await self.stop()
                raise TimeoutError(
                    f"Port {self.port} did not open in time. Server STDOUT: {self.get_stdout()} STDERR: {self.get_stderr()}")
        return self

    async def _read_stream(self, stream, buffer):
        try:
            while not stream.at_eof():
                data = await stream.read(64 * 1024)
                if not data:
                    break
                buffer.write(data.decode(errors='replace'))
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
        if exc_type is not None:
            print(f"\n--- Test Failed/Interrupted ({exc_type.__name__}) ---")
            print(f"Server STDOUT:\n{self.get_stdout()}")
            print(f"Server STDERR:\n{self.get_stderr()}")

        await self.stop()
