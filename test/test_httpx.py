from utils import wait_for_port, run_server
import httpx
import pytest
import signal
import logging

SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"

logging.basicConfig(
    format="%(levelname)s [%(asctime)s] %(name)s - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    level=logging.DEBUG
)


@pytest.mark.asyncio
async def test_httpx_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(URL)

        assert response.status_code == 200
    finally:
        server.terminate()
        server.wait(timeout=10)
