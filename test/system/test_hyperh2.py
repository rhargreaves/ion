import pytest
import asyncio
import ssl
import socket

SERVER_PORT = 8443
from helpers.h2_helpers import create_connection, send_request, close_connection, open_tls_wrapped_socket


@pytest.mark.asyncio
async def test_hyperh2_returns_200(ion_server):
    conn = create_connection(SERVER_PORT)
    resp = send_request(conn, 1)
    assert resp.status == 200
    close_connection(conn)


@pytest.mark.asyncio
async def test_hyperh2_returns_200_for_multiple_requests_over_persistent_connection(ion_server):
    conn = create_connection(SERVER_PORT)
    for i in range(10):
        stream_id = (i * 2) + 1  # must be odd, starting at 1
        resp = send_request(conn, stream_id)
        assert resp.status == 200
    close_connection(conn)


@pytest.mark.asyncio
async def test_server_connection_limit(ion_server):
    max_connections = 128
    active_socks = []

    for i in range(max_connections + 2):
        try:
            s = await asyncio.to_thread(open_tls_wrapped_socket, SERVER_PORT)
            s.setblocking(False)
            active_socks.append(s)
        except Exception as e:
            assert "[SSL: UNEXPECTED_EOF_WHILE_READING] EOF occurred in violation of protocol" in str(e)
            break
    assert len(active_socks) == 128

    # drop one connection and verify that it works again
    active_socks.pop().close()
    conn = create_connection(SERVER_PORT)
    try:
        resp = send_request(conn, 1)
        assert resp.status == 200
    finally:
        close_connection(conn)


@pytest.mark.asyncio
@pytest.mark.timeout(10)
async def test_server_handles_slow_client_read_before_tls_handshake(ion_server):
    slow_socks = []
    num_socks = 5

    for _ in range(num_socks):
        s = socket.create_connection(("localhost", SERVER_PORT))
        s.setblocking(False)
        slow_socks.append(s)

    await asyncio.sleep(7)

    eof_errors = 0
    still_open = 0
    for s in slow_socks:
        try:
            closed = (s.recv(1) == b"")
            eof_errors += 1
            assert closed, "connection should have been closed by server"
        except BlockingIOError:
            still_open += 1
            pass
        except ConnectionResetError:
            eof_errors += 1
            pass
    assert eof_errors == num_socks
    assert still_open == 0


@pytest.mark.skip("wip")
@pytest.mark.asyncio
@pytest.mark.timeout(7)
async def test_server_handles_slow_client_read_after_tls_handshake(ion_server):
    slow_socks = []
    num_socks = 5

    for _ in range(num_socks):
        s = open_tls_wrapped_socket(SERVER_PORT)
        s.setblocking(False)
        slow_socks.append(s)

    await asyncio.sleep(5)

    eof_errors = 0
    still_open = 0
    for s in slow_socks:
        try:
            assert s.recv(1) == b"", "connection should have been closed by server"
        except (BlockingIOError, ssl.SSLWantReadError):
            still_open += 1
            pass
        except (ConnectionResetError, ssl.SSLEOFError):
            eof_errors += 1
            pass
    assert eof_errors == num_socks
    assert still_open == 0
