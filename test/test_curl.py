import pytest

from utils import wait_for_port, curl_http2, assert_curl_response_ok, run_server
import signal

SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"


def test_http2_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        result = curl_http2(URL)

        assert result.returncode == 0
        assert_curl_response_ok(result)

        server.send_signal(signal.SIGTERM)
        server.wait(timeout=10)
    except Exception as e:
        server.terminate()
        server.wait()
        raise e


def test_http2_returns_200_many_times_same_server():
    server = run_server(SERVER_PORT)
    try:
        for run in range(5):
            print(f"------- run {run} -------")
            assert wait_for_port(SERVER_PORT)
            result = curl_http2(URL)
            assert result.returncode == 0
            assert_curl_response_ok(result)

        server.send_signal(signal.SIGTERM)
        server.wait(timeout=20)
    except Exception as e:
        server.terminate()
        server.wait()
        raise e


def test_http2_test_server_closes_connection_cleanly():
    for _ in range(10):
        test_http2_returns_200()
