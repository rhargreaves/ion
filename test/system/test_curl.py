import pytest

from helpers.server_runner import ServerRunner
from helpers.utils import curl_http2, assert_curl_response_ok, curl

SERVER_CLEARTEXT_PORT = 8080
SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"
OK_URL = URL + "/_tests/ok"


@pytest.mark.asyncio
async def test_http2_returns_200():
    async with ServerRunner(SERVER_PORT) as runner:
        result = await curl_http2(OK_URL)

        assert result.returncode == 0
        assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_request_sends_custom_header_with_short_value():
    async with ServerRunner(SERVER_PORT) as runner:
        result = await curl_http2(OK_URL, ["--header", "x-foo: bar"])  # non-huffman compressed value

        assert result.returncode == 0
        assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_request_sends_custom_header_with_long_value():
    async with ServerRunner(SERVER_PORT) as runner:
        result = await curl_http2(OK_URL,
                                  ["--header",
                                   "x-foo: foo bar baz foo bar baz foo bar baz"])  # huffman compressed value

        assert result.returncode == 0
        assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_returns_200_many_times_same_server():
    async with ServerRunner(SERVER_PORT) as runner:
        for run in range(5):
            print(f"------- run {run} -------")
            result = await curl_http2(OK_URL)
            assert result.returncode == 0
            assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_supports_cleartext_http2():
    async with ServerRunner(SERVER_CLEARTEXT_PORT, ["--cleartext", "--under-test"]) as runner:
        result = await curl(f"http://localhost:{SERVER_CLEARTEXT_PORT}/_tests/ok", ["--http2-prior-knowledge"])

        assert result.returncode == 0
        assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_drops_http1_connections():
    async with ServerRunner(SERVER_CLEARTEXT_PORT, ["--cleartext"]) as runner:
        result = await curl(f"http://localhost:{SERVER_CLEARTEXT_PORT}/_tests/ok", ["--http1.1"])
        assert result.returncode == 1

        await runner.stop()
        stdout = runner.get_stdout()
        assert stdout.count("invalid HTTP/2 preface received") == 1


@pytest.mark.asyncio
@pytest.mark.skip("wip")
async def test_curl_returns_large_body():
    async with ServerRunner(SERVER_PORT) as runner:
        result = await curl_http2(URL + "/_tests/large_body")

        assert result.returncode == 0
        assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_test_server_closes_connection_cleanly():
    for _ in range(10):
        await test_http2_returns_200()
