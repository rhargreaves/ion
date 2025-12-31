import socket
import ssl
import certifi

import h2.connection
import h2.events

from dataclasses import dataclass, field
from typing import List, Dict, Any
from time import sleep

SERVER_NAME = 'localhost'


@dataclass
class H2Response:
    status: int | None
    body: str | None
    headers: Dict[str, str] = field(default_factory=dict)
    events: List[Any] = field(default_factory=list)


def open_tls_wrapped_socket(port):
    socket.setdefaulttimeout(15)
    ctx = ssl.create_default_context(cafile=certifi.where())
    ctx.set_alpn_protocols(['h2'])
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    s = socket.create_connection((SERVER_NAME, port))
    return ctx.wrap_socket(s, server_hostname=SERVER_NAME)


def create_connection(port):
    c = h2.connection.H2Connection()
    c.initiate_connection()
    s = open_tls_wrapped_socket(port)
    s.sendall(c.data_to_send())
    return c, s


def close_connection(conn):
    c, s = conn
    c.close_connection()  # send GOAWAY
    s.sendall(c.data_to_send())
    s.close()
    sleep(0.05)


def send_request(conn, stream_id):
    c, s = conn
    headers = [
        (':method', 'GET'),
        (':path', '/_tests/ok'),
        (':authority', SERVER_NAME),
        (':scheme', 'https'),
    ]
    c.send_headers(stream_id, headers, end_stream=True)
    s.sendall(c.data_to_send())

    status_code = None
    headers_dict = {}
    body = b''
    all_events = []
    response_stream_ended = False

    while not response_stream_ended:
        data = s.recv(64 * 1024)
        if not data:
            break

        events = c.receive_data(data)
        for event in events:
            all_events.append(event)

            if isinstance(event, h2.events.ResponseReceived):
                headers_dict = {k.decode(): v.decode() for k, v in event.headers}
                status_code = headers_dict.get(':status')

            if isinstance(event, h2.events.DataReceived):
                c.acknowledge_received_data(event.flow_controlled_length, event.stream_id)
                body += event.data

            if isinstance(event, h2.events.StreamEnded):
                response_stream_ended = True
                break

        s.sendall(c.data_to_send())

    return H2Response(
        status=int(status_code),
        body=body.decode() if body else None,
        headers=headers_dict,
        events=all_events
    )
