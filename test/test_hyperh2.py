import socket
import ssl
import certifi

import h2.connection
import h2.events
import pytest

from utils import wait_for_port, run_server

SERVER_NAME = 'localhost'
SERVER_PORT = 8443


def open_tls_wrapped_socket():
    socket.setdefaulttimeout(15)
    ctx = ssl.create_default_context(cafile=certifi.where())
    ctx.set_alpn_protocols(['h2'])
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    s = socket.create_connection((SERVER_NAME, SERVER_PORT))
    return ctx.wrap_socket(s, server_hostname=SERVER_NAME)


def make_request():
    c = h2.connection.H2Connection()
    c.initiate_connection()
    s = open_tls_wrapped_socket()
    s.sendall(c.data_to_send())

    headers = [
        (':method', 'GET'),
        (':path', '/'),
        (':authority', SERVER_NAME),
        (':scheme', 'https'),
    ]
    c.send_headers(1, headers, end_stream=True)
    s.sendall(c.data_to_send())

    body = b''
    response_stream_ended = False
    while not response_stream_ended:
        # read raw data from the socket
        data = s.recv(65536 * 1024)
        if not data:
            break

        # feed raw data into h2, and process resulting events
        events = c.receive_data(data)
        for event in events:
            print(event)
            if isinstance(event, h2.events.DataReceived):
                # update flow control so the server doesn't starve us
                c.acknowledge_received_data(event.flow_controlled_length, event.stream_id)
                # more response body data received
                body += event.data
            if isinstance(event, h2.events.StreamEnded):
                # response body completed, let's exit the loop
                response_stream_ended = True
                break
        # send any pending data to the server
        s.sendall(c.data_to_send())

    c.close_connection()  # send GOAWAY
    s.sendall(c.data_to_send())
    s.close()
    return body.decode()


def test_hyperh2_returns_200():
    server = run_server(SERVER_PORT)
    assert wait_for_port(SERVER_PORT)
    try:
        output = make_request()
        print(output)
    finally:
        server.terminate()
        server.wait(timeout=10)
        assert server.returncode == 0
