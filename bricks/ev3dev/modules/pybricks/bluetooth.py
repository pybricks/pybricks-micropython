# SPDX-License-Identifier: MIT
# Copyright (C) 2020 David Lechner

"""This module provides :class`EV3MailboxServer` and :class:`EV3MailboxClient`
which are compatible with the official EV3 firmware mailboxes.

It is based on the standard library ``socketserver`` module and attempts to
remain a strict subset of that implementation when it comes to low-level
implementation details.

:class:`RFCOMMServer` can be used to communicate with other Bluetooth RFCOMM
devices that don't support the EV3 mailbox protocol.
"""

from _thread import start_new_thread, allocate_lock
from uerrno import ECONNRESET
from uctypes import addressof, sizeof, struct, ARRAY, UINT8, UINT16
from usocket import socket, SOCK_STREAM
from ustruct import pack, unpack

# MicroPython doesn't support __all__ yet but this still documents the intended
# public members
__all__ = ['BDADDR_ANY', 'RFCOMMServer', 'ThreadingRFCOMMServer',
           'StreamRequestHandler', 'EV3MailboxServer', 'EV3MailboxClient']

# stuff from bluetooth/bluetooth.h

AF_BLUETOOTH = 31
BTPROTO_RFCOMM = 3
BDADDR_ANY = '00:00:00:00:00:00'

sa_family_t = UINT16

bd_addr_t = {
    'b': (ARRAY | 0, UINT8 | 6)
}

sockaddr_rc = {
    'rc_family': sa_family_t | 0,
    'rc_bdaddr': (2, bd_addr_t),
    'rc_channel': UINT8 | 8,
}


def str2ba(string, ba):
    """Convert string to Bluetooth address"""
    for i, v in enumerate(string.split(':')):
        ba.b[5-i] = int(v, 16)


def ba2str(ba):
    """Convert Bluetooth address to string"""
    string = []
    for b in ba.b:
        string.append('{:02X}'.format(b))
    string.reverse()
    return ':'.join(string).upper()


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
            request, client_address = self.socket.accept()
        except OSError:
            return

        try:
            self.process_request(request, client_address)
        except:
            request.close()
            raise

    def process_request(self, request, client_address):
        self.RequestHandlerClass(request, client_address, self)
        request.close()

    def server_close(self):
        self.socket.close()


class ThreadingMixIn:
    def process_request_thread(self, request, client_address):
        try:
            self.RequestHandlerClass(request, client_address, self)
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


EV3_RFCOMM_CHANNEL = 1  # EV3 standard firmware is hard-coded to use channel 1
# EV3 VM bytecodes
SYSTEM_COMMAND_NO_REPLY = 0x81
WRITEMAILBOX = 0x9E


class EV3MailboxHandler(StreamRequestHandler):
    def handle(self):
        with self.server._lock:
            addr = struct(addressof(self.client_address), sockaddr_rc)
            self.server._clients[ba2str(addr.rc_bdaddr)] = self.request
        while True:
            try:
                buf = self.rfile.read(2)
            except OSError as ex:
                # The client disconnected the connection
                if ex.args[0] == ECONNRESET:
                    break
                raise
            size, = unpack('<H', buf)
            buf = self.rfile.read(size)
            msg_count, cmd_type, cmd, name_size = unpack('<HBBB', buf)
            if cmd_type != SYSTEM_COMMAND_NO_REPLY:
                raise ValueError('Bad message type')
            if cmd != WRITEMAILBOX:
                raise ValueError('Bad command')
            mbox = buf[5:5+name_size].decode().strip('\0')
            data_size, = unpack('<H', buf[5+name_size:7+name_size])
            data = buf[7+name_size:7+name_size+data_size]

            with self.server._lock:
                self.server._mailboxes[mbox] = data
                update_lock = self.server._updates.get(mbox)
                if update_lock:
                    update_lock.release()


class EV3MailboxMixIn:
    def __init__(self):
        self._lock = allocate_lock()
        self._mailboxes = {}
        self._clients = {}
        self._updates = {}

    def read_from_mailbox(self, mbox):
        """Reads the current raw data from a mailbox.

        Arguments:
            mbox (str):
                The name of the mailbox.

        Returns:
            bytes:
                The current mailbox raw data or ``None`` if nothing has ever
                been delivered to the mailbox.
        """
        with self._lock:
            return self._mailboxes.get(mbox)

    def send_to_mailbox(self, brick, mbox, payload):
        """Sends a mailbox value using raw bytes data.

        .. todo:: Currently the Bluetooth address must be used instead of the
                  the brick name.

        Arguments:
            brick (str):
                The name of the brick or ``None``` to broadcast
            mbox (str):
                The name of the mailbox.
            payload (bytes):
                A bytes-like object that will be sent to the mailbox.
        """
        mbox_len = len(mbox) + 1
        payload_len = len(payload)
        send_len = 7 + mbox_len + payload_len
        fmt = '<HHBBB{}sH{}s'.format(mbox_len, payload_len)
        data = pack(fmt, send_len, 1, SYSTEM_COMMAND_NO_REPLY, WRITEMAILBOX,
                    mbox_len, mbox, payload_len, payload)
        with self._lock:
            if brick is None:
                for client in self._clients.values():
                    client.send(data)
            else:
                self._clients[brick].send(data)

    def wait_for_mailbox_update(self, mbox):
        """Waits until ``mbox`` receives a value."""
        lock = allocate_lock()
        lock.acquire()
        with self._lock:
            self._updates[mbox] = lock
        try:
            return lock.acquire()
        finally:
            with self._lock:
                del self._updates[mbox]


class EV3MailboxServer(EV3MailboxMixIn, ThreadingRFCOMMServer):
    def __init__(self):
        """Object that represents an incoming Bluetooth connection from another
        EV3.

        The remote EV3 can either be running MicroPython or the standard EV3
        firmare.
        """
        super().__init__()
        super(ThreadingRFCOMMServer, self).__init__(
            (BDADDR_ANY, EV3_RFCOMM_CHANNEL), EV3MailboxHandler)

    def wait_for_connection(self, count=1):
        """Waits for a :class:`EV3MailboxClient` on a remote device to connect.

        Arguments:
            count (int):
                The number of remote connections to wait for.

        Raises:
            OSError:
                There was a problem establishing the connection.
        """
        for _ in range(count):
            self.handle_request()


class EV3MailboxClient(EV3MailboxMixIn, ThreadingMixIn):
    def __init__(self, brick):
        """Object that represents an outgoing Bluetooth connection to another
        EV3.

        The remote EV3 can either be running MicroPython or the standard EV3
        firmare.

        .. todo:: Currently the Bluetooth address must be used instead of the
                  the brick name.

        Arguments:
            brick (str):
                The name of the remote EV3 to connect to.
        """
        super().__init__()
        addr = brick  # FIXME: resolve name to address
        self.client_address = (addr, EV3_RFCOMM_CHANNEL)
        self.socket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)
        self.RequestHandlerClass = EV3MailboxHandler

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def connect(self):
        """Connects to a :class:`EV3MailboxServer` on another device.

        The remote device must be paired and waiting for a connection. See
        :meth:`EV3MailboxServer.wait_for_connection`.

        Raises:
            OSError:
                There was a problem establishing the connection.
                ``OSError: 16`` is
        """
        addr_data = bytearray(sizeof(sockaddr_rc))
        addr = struct(addressof(addr_data), sockaddr_rc)
        addr.rc_family = AF_BLUETOOTH
        str2ba(self.client_address[0], addr.rc_bdaddr)
        addr.rc_channel = self.client_address[1]
        self.socket.connect(addr_data)
        self._clients[self.client_address[0]] = self.socket
        self.process_request(self.socket, addr)

    def close(self):
        """Closes the connection."""
        self.socket.close()
