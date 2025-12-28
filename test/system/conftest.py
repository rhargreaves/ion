import pytest
import pytest_asyncio
from helpers.server_runner import ServerRunner

DEFAULT_SERVER_PORT = 8443


@pytest_asyncio.fixture
async def ion_server(request):
    param = getattr(request, "param", {})
    port = DEFAULT_SERVER_PORT
    server_args = None
    extra_args = None

    if isinstance(param, list):
        extra_args = param
    elif isinstance(param, dict):
        port = param.get("port", DEFAULT_SERVER_PORT)
        server_args = param.get("args")
        extra_args = param.get("extra_args")

    async with ServerRunner(port, args=server_args, extra_args=extra_args) as runner:
        yield runner
