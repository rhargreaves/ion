import pytest
import pytest_asyncio
from helpers.server_runner import ServerRunner


@pytest_asyncio.fixture
async def ion_server(request):
    # Get args and port from parametrization
    param = getattr(request, "param", {})
    port = 8443
    server_args = None
    extra_args = None

    if isinstance(param, list):
        extra_args = param
    elif isinstance(param, dict):
        port = param.get("port", 8443)
        server_args = param.get("args")
        extra_args = param.get("extra_args")

    runner = ServerRunner(port, args=server_args, extra_args=extra_args)
    # The __aenter__ logic usually starts the process
    await runner.__aenter__()

    try:
        yield runner
    finally:
        # Guaranteed cleanup even on timeout or test failure
        await runner.__aexit__(None, None, None)
