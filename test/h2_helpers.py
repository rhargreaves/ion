import socket
import ssl
import certifi

import h2.connection
import h2.events

SERVER_NAME = 'localhost'


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


def send_request(conn, stream_id):
    c, s = conn
    headers = [
        (':method', 'GET'),
        (':path', '/'),
        (':authority', SERVER_NAME),
        (':scheme', 'https'),
    ]
    c.send_headers(stream_id, headers, end_stream=True)
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

    return body.decode()
