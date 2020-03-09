# SPDX-License-Identifier: MIT
# Copyright (C) 2020 The Pybricks Authors

"""
:class:`RFCOMMServer` can be used to communicate with other Bluetooth RFCOMM
devices that don't support the EV3 mailbox protocol.

It is based on the standard library ``socketserver`` module and attempts to
remain a strict subset of that implementation when it comes to low-level
implementation details.
"""

from _thread import start_new_thread
from uctypes import addressof, sizeof, struct, ARRAY, UINT8, UINT16
from usocket import socket, SOCK_STREAM

from bluetooth_c import resolve

# stuff from bluetooth/bluetooth.h

AF_BLUETOOTH = 31
BTPROTO_RFCOMM = 3
BDADDR_ANY = "00:00:00:00:00:00"

sa_family_t = UINT16

bd_addr_t = {"b": (ARRAY | 0, UINT8 | 6)}

sockaddr_rc = {
    "rc_family": sa_family_t | 0,
    "rc_bdaddr": (2, bd_addr_t),
    "rc_channel": UINT8 | 8,
}


def str2ba(string, ba):
    """Convert string to Bluetooth address"""
    for i, v in enumerate(string.split(":")):
        ba.b[5 - i] = int(v, 16)


def ba2str(ba):
    """Convert Bluetooth address to string"""
    string = []
    for b in ba.b:
        string.append("{:02X}".format(b))
    string.reverse()
    return ":".join(string).upper()


class RFCOMMServer:
    """Object that simplifies setting up an RFCOMM socket server.

    This is based on the ``socketserver.SocketServer`` class in the Python
    standard library.
    """

    request_queue_size = 1

    def __init__(self, server_address, RequestHandlerClass):
        self.server_address = server_address
        self.RequestHandlerClass = RequestHandlerClass

        self.socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)

        try:
            addr_data = bytearray(sizeof(sockaddr_rc))
            addr = struct(addressof(addr_data), sockaddr_rc)
            addr.rc_family = AF_BLUETOOTH
            str2ba(server_address[0], addr.rc_bdaddr)
            addr.rc_channel = server_address[1]
            self.socket.bind(addr_data)
            # self.server_address = self.socket.getsockname()
            self.socket.listen(self.request_queue_size)
        except:
            self.server_close()
            raise

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.server_close()

    def handle_request(self):
        try:
            request, addr_data = self.socket.accept()
        except OSError:
            return

        try:
            addr = struct(addressof(addr_data), sockaddr_rc)
            client_address = (ba2str(addr.rc_bdaddr), addr.rc_channel)
            self.process_request(request, client_address)
        except:
            request.close()
            raise

    def process_request(self, request, client_address):
        self.finish_request(request, client_address)
        request.close()

    def finish_request(self, request, client_address):
        self.RequestHandlerClass(request, client_address, self)

    def server_close(self):
        self.socket.close()


class ThreadingMixIn:
    def process_request_thread(self, request, client_address):
        try:
            self.finish_request(request, client_address)
        finally:
            request.close()

    def process_request(self, request, client_address):
        start_new_thread(self.process_request_thread, (request, client_address))


class ThreadingRFCOMMServer(ThreadingMixIn, RFCOMMServer):
    """Version of :class:`RFCOMMServer` that handles connections in a new
    thread.
    """

    pass


class StreamRequestHandler:
    """Class that handles incoming requests.

    This is based on ``socketserver.StreamRequestHandler`` from the Python
    standard library.
    """

    def __init__(self, request, client_address, server):
        self.request = request
        self.client_address = client_address
        self.server = server
        self.setup()
        try:
            self.handle()
        finally:
            self.finish()

    def setup(self):
        self.wfile = self.request
        self.rfile = self.request

    def handle(self):
        pass

    def finish(self):
        pass


class RFCOMMClient:
    def __init__(self, client_address, RequestHandlerClass):
        self.client_address = client_address
        self.RequestHandlerClass = RequestHandlerClass
        self.socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)

    def handle_request(self):
        addr_data = bytearray(sizeof(sockaddr_rc))
        addr = struct(addressof(addr_data), sockaddr_rc)
        addr.rc_family = AF_BLUETOOTH
        str2ba(self.client_address[0], addr.rc_bdaddr)
        addr.rc_channel = self.client_address[1]
        self.socket.connect(addr_data)
        try:
            self.process_request(self.socket, self.client_address)
        except:
            self.socket.close()
            raise

    def process_request(self, request, client_address):
        self.finish_request(request, client_address)
        request.close()

    def finish_request(self, request, client_address):
        self.RequestHandlerClass(request, client_address, self)

    def client_close(self):
        self.socket.close()


class ThreadingRFCOMMClient(ThreadingMixIn, RFCOMMClient):
    pass
