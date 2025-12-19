from utils import wait_for_port, run_server, stop_server, ACCESS_LOG_PATH, assert_last_log_line_is_valid
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
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        resp1 = await client.get(OK_URL, headers={"foo": "bar"})
        assert resp1.status_code == 200

        resp2 = await client.get(OK_URL, headers={"foo": "baz"})
        assert resp2.status_code == 200
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_returns_server_header():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)

        assert response.status_code == 200
        assert response.headers["server"].startswith("ion/")
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)

        assert response.status_code == 200
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_returns_404_for_invalid_path():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/invalid-path")

        assert response.status_code == 404
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_returns_204_for_no_content():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/_tests/no_content")

        assert response.status_code == 204
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_returns_static_content():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(BASE_URL + "/static/index.html")

        assert response.status_code == 200
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_returns_200_for_multiple_requests_over_persistent_connection():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        for i in range(10):
            print(f"request {i}:")
            response = await client.get(OK_URL)
            assert response.status_code == 200
    finally:
        stop_server(server)


@pytest.mark.asyncio
async def test_httpx_logs_requests_in_access_logs():
    server = run_server(SERVER_PORT, extra_args=["--access-log", ACCESS_LOG_PATH])
    assert wait_for_port(SERVER_PORT)
    try:
        client = httpx.AsyncClient(http2=True, verify=False)
        response = await client.get(OK_URL)
        assert response.status_code == 200

        assert_last_log_line_is_valid(ACCESS_LOG_PATH)

    finally:
        stop_server(server)
        os.remove(ACCESS_LOG_PATH)
