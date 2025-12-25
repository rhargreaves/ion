from utils import ServerRunner, assert_last_log_line_is_valid
import httpx
import pytest
import logging
import os

SERVER_PORT = 8443
BASE_URL = f"https://localhost:{SERVER_PORT}"
OK_URL = BASE_URL + "/_tests/ok"

logging.basicConfig(
    format="%(levelname)s [%(asctime)s] %(name)s - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
    level=logging.DEBUG
)


# hpack_logger = logging.getLogger("hpack")
# hpack_logger.setLevel(logging.DEBUG)
# hpack_logger.propagate = True
# logging.getLogger("httpx").setLevel(logging.DEBUG)
# logging.getLogger("httpcore").setLevel(logging.DEBUG)


@pytest.mark.asyncio
async def test_httpx_sends_custom_header_two_different_values():  # support dynamic table name lookup & decoded value
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        resp1 = await client.get(OK_URL, headers={"foo": "bar"})
        assert resp1.status_code == 200

        resp2 = await client.get(OK_URL, headers={"foo": "baz"})
        assert resp2.status_code == 200


@pytest.mark.asyncio
async def test_httpx_returns_server_header():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)

        assert response.status_code == 200
        assert response.headers["server"] == "ion"


@pytest.mark.asyncio
async def test_httpx_returns_200():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)

        assert response.status_code == 200


@pytest.mark.asyncio
async def test_httpx_returns_404_for_invalid_path():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/invalid-path")

        assert response.status_code == 404


@pytest.mark.asyncio
async def test_httpx_returns_204_for_no_content():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/_tests/no_content")

        assert response.status_code == 204


@pytest.mark.asyncio
async def test_httpx_returns_static_content():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/static/index.html")

        assert response.status_code == 200


@pytest.mark.asyncio
async def test_httpx_returns_custom_404_page():
    async with ServerRunner(SERVER_PORT, [
        "--custom-404-file-path",
        os.path.join(os.path.dirname(__file__), "static/404.html")
    ]) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/foo")

        assert response.status_code == 404
        assert response.headers["content-type"] == "text/html; charset=utf-8"
        assert "404 :(" in response.text


@pytest.mark.asyncio
async def test_httpx_returns_medium_body():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/_tests/medium_body")

        assert response.status_code == 200
        assert len(response.content) == 128 * 1024


@pytest.mark.skip("wip")
@pytest.mark.asyncio
async def test_httpx_returns_large_body():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/_tests/large_body")

        assert response.status_code == 200
        assert len(response.content) == 16 * 1024 * 1024


@pytest.mark.asyncio
async def test_httpx_returns_200_for_multiple_requests_over_persistent_connection():
    async with ServerRunner(SERVER_PORT) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        for i in range(10):
            print(f"request {i}:")
            response = await client.get(OK_URL)
            assert response.status_code == 200


@pytest.mark.asyncio
async def test_httpx_logs_requests_in_access_logs():
    async with ServerRunner(SERVER_PORT, extra_args=["--access-log", "/dev/stderr"]) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)
        assert response.status_code == 200

        await runner.stop()
        assert_last_log_line_is_valid(runner.get_stderr())


@pytest.mark.asyncio
async def test_httpx_test_routes_not_available_if_not_under_test():
    async with ServerRunner(SERVER_PORT, args=[]) as runner:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)
        assert response.status_code == 404
