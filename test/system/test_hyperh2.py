import pytest

SERVER_PORT = 8443
from helpers.h2_helpers import create_connection, send_request, close_connection

from helpers.server_runner import ServerRunner


@pytest.mark.asyncio
async def test_hyperh2_returns_200():
    async with ServerRunner(SERVER_PORT) as runner:
        conn = create_connection(SERVER_PORT)
        output = send_request(conn, 1)
        print(output)
        close_connection(conn)


@pytest.mark.asyncio
async def test_hyperh2_returns_200_for_multiple_requests_over_persistent_connection():
    async with ServerRunner(SERVER_PORT) as runner:
        conn = create_connection(SERVER_PORT)
        for i in range(10):
            stream_id = (i * 2) + 1  # must be odd, starting at 1
            output = send_request(conn, stream_id)
            print(output)
        close_connection(conn)
