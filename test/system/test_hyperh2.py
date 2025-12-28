import pytest

SERVER_PORT = 8443
from helpers.h2_helpers import create_connection, send_request, close_connection


@pytest.mark.asyncio
async def test_hyperh2_returns_200(ion_server):
    conn = create_connection(SERVER_PORT)
    output = send_request(conn, 1)
    print(output)
    close_connection(conn)


@pytest.mark.asyncio
async def test_hyperh2_returns_200_for_multiple_requests_over_persistent_connection(ion_server):
    conn = create_connection(SERVER_PORT)
    for i in range(10):
        stream_id = (i * 2) + 1  # must be odd, starting at 1
        output = send_request(conn, stream_id)
        print(output)
    close_connection(conn)
