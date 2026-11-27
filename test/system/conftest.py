import os

import pytest_asyncio
from helpers.server_runner import ServerRunner
from helpers.worker_runner import WorkerRunner

DEFAULT_SERVER_PORT = 8443
USE_WORKER = os.environ.get("ION_WASM") == "1"


@pytest_asyncio.fixture
async def ion_server(request):
    param = getattr(request, "param", {})
    port = DEFAULT_SERVER_PORT
    server_args = None
    extra_args = None
    env = None

    if isinstance(param, list):
        extra_args = param
    elif isinstance(param, dict):
        port = param.get("port", DEFAULT_SERVER_PORT)
        server_args = param.get("args")
        extra_args = param.get("extra_args")
        env = param.get("env")

    if USE_WORKER:
        runner = WorkerRunner(port, extra_args=extra_args, env=env)
    else:
        runner = ServerRunner(port, args=server_args, extra_args=extra_args, env=env)

    async with runner as running:
        yield running
