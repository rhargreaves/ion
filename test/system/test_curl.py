import pytest

from utils import wait_for_port, curl_http2, assert_curl_response_ok, run_server, stop_server

SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"
OK_URL = URL + "/_tests/ok"


def test_http2_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        result = curl_http2(OK_URL)

        assert result.returncode == 0
        assert_curl_response_ok(result)
    finally:
        stop_server(server)


def test_http2_request_sends_custom_header_with_short_value():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        result = curl_http2(OK_URL, ["--header", "x-foo: bar"])  # non-huffman compressed value

        assert result.returncode == 0
        assert_curl_response_ok(result)
    finally:
        stop_server(server)


def test_http2_request_sends_custom_header_with_long_value():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        result = curl_http2(OK_URL,
                            ["--header", "x-foo: foo bar baz foo bar baz foo bar baz"])  # huffman compressed value

        assert result.returncode == 0
        assert_curl_response_ok(result)
    finally:
        stop_server(server)


def test_http2_returns_200_many_times_same_server():
    server = run_server(SERVER_PORT)
    try:
        for run in range(5):
            print(f"------- run {run} -------")
            assert wait_for_port(SERVER_PORT)
            result = curl_http2(OK_URL)
            assert result.returncode == 0
            assert_curl_response_ok(result)
    finally:
        stop_server(server)


def test_http2_test_server_closes_connection_cleanly():
    for _ in range(10):
        test_http2_returns_200()
