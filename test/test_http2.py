from utils import wait_for_port, curl_http2, assert_curl_response_ok, run_server

SERVER_PORT = 8443
URL = f"https://localhost:{SERVER_PORT}"


def test_http2_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        result = curl_http2(URL)

        assert result.returncode == 0
        assert_curl_response_ok(result)
    finally:
        server.terminate()
        server.wait()


def test_http2_returns_200_many_times():
    for _ in range(10):
        test_http2_returns_200()
