import pytest

SERVER_PORT = 8443
from h2_helpers import create_connection, send_request, close_connection

from utils import wait_for_port, run_server


def test_hyperh2_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        conn = create_connection(SERVER_PORT)
        output = send_request(conn, 1)
        print(output)
        close_connection(conn)
    finally:
        server.terminate()
        server.wait(timeout=10)
        assert server.returncode == 0


def test_hyperh2_returns_200_for_multiple_requests_over_persistent_connection():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        conn = create_connection(SERVER_PORT)
        for i in range(10):
            stream_id = (i * 2) + 1  # must be odd, starting at 1
            output = send_request(conn, stream_id)
            print(output)
        close_connection(conn)
    finally:
        server.terminate()
        server.wait(timeout=10)
        assert server.returncode == 0
