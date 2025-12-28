import pytest

from helpers.utils import curl_http2, assert_curl_response_ok, curl

SERVER_CLEARTEXT_PORT = 8080
SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"
OK_URL = URL + "/_tests/ok"


@pytest.mark.asyncio
async def test_http2_returns_200(ion_server):
    result = await curl_http2(OK_URL)

    assert result.returncode == 0
    assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_request_sends_custom_header_with_short_value(ion_server):
    result = await curl_http2(OK_URL, ["--header", "x-foo: bar"])  # non-huffman compressed value

    assert result.returncode == 0
    assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_request_sends_custom_header_with_long_value(ion_server):
    result = await curl_http2(OK_URL,
                              ["--header",
                               "x-foo: foo bar baz foo bar baz foo bar baz"])  # huffman compressed value

    assert result.returncode == 0
    assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_returns_200_many_times_same_server(ion_server):
    for run in range(5):
        print(f"------- run {run} -------")
        result = await curl_http2(OK_URL)
        assert result.returncode == 0
        assert_curl_response_ok(result)


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [{"port": SERVER_CLEARTEXT_PORT, "args": ["--cleartext", "--under-test"]}],
                         indirect=True)
async def test_supports_cleartext_http2(ion_server):
    result = await curl(f"http://localhost:{SERVER_CLEARTEXT_PORT}/_tests/ok", ["--http2-prior-knowledge"])

    assert result.returncode == 0
    assert_curl_response_ok(result)


@pytest.mark.asyncio
@pytest.mark.parametrize("ion_server", [{"port": SERVER_CLEARTEXT_PORT, "args": ["--cleartext"]}], indirect=True)
async def test_drops_http1_connections(ion_server):
    result = await curl(f"http://localhost:{SERVER_CLEARTEXT_PORT}/_tests/ok", ["--http1.1"])
    assert result.returncode == 52

    await ion_server.stop()
    stdout = ion_server.get_stdout()
    assert stdout.count("invalid HTTP/2 preface received") == 1


@pytest.mark.asyncio
async def test_curl_returns_large_body(ion_server):
    result = await curl_http2(URL + "/_tests/large_body")

    print("server stdout: " + ion_server.get_stdout())
    assert result.returncode == 0
    assert_curl_response_ok(result)


@pytest.mark.asyncio
async def test_http2_test_server_closes_connection_cleanly(ion_server):
    for _ in range(10):
        result = await curl_http2(OK_URL)
        assert result.returncode == 0
        assert_curl_response_ok(result)
