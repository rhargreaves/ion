from helpers.utils import assert_last_log_line_is_valid, DEFAULT_ARGS, OK_URL, BASE_URL
import httpx
import pytest
import logging
import os

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
async def test_httpx_sends_custom_header_two_different_values(
    ion_server):  # support dynamic table name lookup & decoded value
    client = httpx.AsyncClient(http2=True, verify=False)
    resp1 = await client.get(OK_URL, headers={"foo": "bar"})
    assert resp1.status_code == 200

    resp2 = await client.get(OK_URL, headers={"foo": "baz"})
    assert resp2.status_code == 200


@pytest.mark.asyncio
async def test_httpx_returns_server_header(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(OK_URL)

    assert response.status_code == 200
    assert response.headers["server"] == "ion"


@pytest.mark.asyncio
async def test_httpx_returns_200(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(OK_URL)

    assert response.status_code == 200


@pytest.mark.asyncio
async def test_httpx_returns_404_for_invalid_path(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/invalid-path")

    assert response.status_code == 404


@pytest.mark.asyncio
async def test_httpx_returns_204_for_no_content(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/_tests/no_content")

    assert response.status_code == 204


@pytest.mark.asyncio
async def test_httpx_returns_static_content(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/static/index.html")

    assert response.status_code == 200


@pytest.mark.asyncio
async def test_httpx_supports_head_requests_on_static_content(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.head(BASE_URL + "/static/index.html")

    assert response.status_code == 200


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [[
    "--custom-404-file-path",
    os.path.join(os.path.dirname(__file__), "static/404.html")
]], indirect=True)
async def test_httpx_returns_custom_404_page(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/foo")

    assert response.status_code == 404
    assert response.headers["content-type"] == "text/html; charset=utf-8"
    assert "404 :(" in response.text


@pytest.mark.asyncio
async def test_httpx_returns_medium_body(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/_tests/medium_body")

    assert response.status_code == 200
    assert len(response.content) == 128 * 1024


@pytest.mark.asyncio
@pytest.mark.timeout(5)
async def test_httpx_returns_large_body(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/_tests/large_body")

    assert response.status_code == 200
    assert len(response.content) == 2 * 1024 * 1024


@pytest.mark.asyncio
async def test_httpx_returns_200_for_multiple_requests_over_persistent_connection(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    for i in range(10):
        print(f"request {i}:")
        response = await client.get(OK_URL)
        assert response.status_code == 200


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [["--access-log", "/dev/stderr"]], indirect=True)
async def test_httpx_logs_requests_in_access_logs(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(OK_URL)
    assert response.status_code == 200

    await ion_server.stop()
    assert_last_log_line_is_valid(ion_server.get_stderr())


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [{"args": [arg for arg in DEFAULT_ARGS if arg != "--under-test"]}],
                         indirect=True)
async def test_httpx_test_routes_not_available_if_not_under_test(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(OK_URL)
    assert response.status_code == 404


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [["--status-page"]], indirect=True)
async def test_httpx_status_page_shows_server_status(ion_server):
    client = httpx.AsyncClient(http2=True, verify=False)
    response = await client.get(BASE_URL + "/_ion/status")

    assert response.status_code == 200
    assert response.headers["content-type"] == "text/html; charset=utf-8"
    assert response.headers["cache-control"] == "no-store"
