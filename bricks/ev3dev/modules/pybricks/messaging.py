# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

from _thread import allocate_lock
from uerrno import ECONNRESET
from ustruct import pack, unpack

from pybricks.bluetooth import (
    resolve,
    BDADDR_ANY,
    ThreadingRFCOMMServer,
    ThreadingRFCOMMClient,
    StreamRequestHandler,
)


class Mailbox:
    def __init__(self, name, connection, encode=None, decode=None):
        """Object that represents a mailbox for sending an receiving messages
        from other connected devices.

        Arguments:
            name (str):
                The name of this mailbox.
            connection:
                A connection object that implements the mailbox connection
                interface.
            encode:
                A function that encodes an object into a bytes-like object.
            decode:
                A function that decodes an object from a bytes-like object.
        """
        self.name = name
        self._connection = connection

        if encode:
            self.encode = encode

        if decode:
            self.decode = decode

    def encode(self, value):
        return value

    def decode(self, payload):
        return payload

    def read(self):
        """Reads the current value of the mailbox.

        Returns:
            The decoded value or ``None`` if the mailbox has never received
            a value.
        """
        data = self._connection.read_from_mailbox(self.name)
        if data is None:
            return None
        return self.decode(data)

    def send(self, value, destination=None):
        """Sends a value to remote mailboxes with the same name as this
        mailbox.

        Arguments:
            value: The value to send.
            destination: The name or address of a specific device or ``None``
                to broadcast to all connected devices.
        """
        data = self.encode(value)
        self._connection.send_to_mailbox(destination, self.name, data)

    def wait(self):
        """Waits for the mailbox to receive a message."""
        self._connection.wait_for_mailbox_update(self.name)

    def wait_new(self):
        """Waits for the mailbox to receive a message that is different from
        the current contents of the mailbox.

        Returns:
            The new value. (Same as return value of :meth:`read`.)
        """
        old = self.read()
        while True:
            self.wait()
            new = self.read()
            if new != old:
                return new


class LogicMailbox(Mailbox):
    """:class:`Mailbox` that holds a logic or boolean value.

    This is compatible with the "logic" message blocks in the standard
    EV3 firmware.
    """

    def encode(self, value):
        return b"\x01" if value else b"\x00"

    def decode(self, payload):
        return bool(payload[0])


class NumericMailbox(Mailbox):
    """:class:`Mailbox` that holds a numeric or floating point value.

    This is compatible with the "numeric" message blocks in the standard
    EV3 firmware.
    """

    def encode(self, value):
        return pack("<f", value)

    def decode(self, payload):
        return unpack("<f", payload)[0]


class TextMailbox(Mailbox):
    """:class:`Text` that holds a text or string point value.

    This is compatible with the "text" message blocks in the standard
    EV3 firmware.
    """

    def encode(self, value):
        return "{}\0".format(value)

    def decode(self, payload):
        return payload.decode().strip("\0")


# EV3 standard firmware is hard-coded to use channel 1
EV3_RFCOMM_CHANNEL = 1

# EV3 VM bytecodes
SYSTEM_COMMAND_NO_REPLY = 0x81
WRITEMAILBOX = 0x9E


class MailboxHandler(StreamRequestHandler):
    def handle(self):
        with self.server._lock:
            self.server._clients[self.client_address[0]] = self.request
        while True:
            try:
                buf = self.rfile.read(2)
            except OSError as ex:
                # The client disconnected the connection
                if ex.errno == ECONNRESET:
                    break
                raise
            (size,) = unpack("<H", buf)
            buf = self.rfile.read(size)
            msg_count, cmd_type, cmd, name_size = unpack("<HBBB", buf)
            if cmd_type != SYSTEM_COMMAND_NO_REPLY:
                raise ValueError("Bad message type")
            if cmd != WRITEMAILBOX:
                raise ValueError("Bad command")
            mbox = buf[5 : 5 + name_size].decode().strip("\0")
            (data_size,) = unpack("<H", buf[5 + name_size : 7 + name_size])
            data = buf[7 + name_size : 7 + name_size + data_size]

            with self.server._lock:
                self.server._mailboxes[mbox] = data
                update_lock = self.server._updates.get(mbox)
                if update_lock:
                    update_lock.release()


class MailboxHandlerMixIn:
    def __init__(self):
        # protects against concurrent access of other attributes
        self._lock = allocate_lock()
        # map of mailbox name to raw data
        self._mailboxes = {}
        # map of device name/address to object with send() method
        self._clients = {}
        # map of mailbox name to mutex lock
        self._updates = {}
        # map of names to addresses
        self._addresses = {}

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

        Arguments:
            brick (str):
                The name or address of the brick or ``None``` to broadcast to
                all connected devices
            mbox (str):
                The name of the mailbox.
            payload (bytes):
                A bytes-like object that will be sent to the mailbox.
        """
        mbox_len = len(mbox) + 1
        payload_len = len(payload)
        send_len = 7 + mbox_len + payload_len
        fmt = "<HHBBB{}sH{}s".format(mbox_len, payload_len)
        data = pack(
            fmt,
            send_len,
            1,
            SYSTEM_COMMAND_NO_REPLY,
            WRITEMAILBOX,
            mbox_len,
            mbox,
            payload_len,
            payload,
        )
        with self._lock:
            if brick is None:
                for client in self._clients.values():
                    client.send(data)
            else:
                addr = self._addresses.get(brick)
                if addr is None:
                    addr = resolve(brick)
                    self._addresses[brick] = addr
                if addr is None:
                    raise ValueError('no paired devices matching "{}"'.format(brick))
                self._clients[addr].send(data)

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


class BluetoothMailboxServer(MailboxHandlerMixIn, ThreadingRFCOMMServer):
    def __init__(self):
        """Object that represents an incoming Bluetooth connection from another
        EV3.

        The remote EV3 can either be running MicroPython or the standard EV3
        firmare.
        """
        super().__init__()
        super(ThreadingRFCOMMServer, self).__init__(
            (BDADDR_ANY, EV3_RFCOMM_CHANNEL), MailboxHandler
        )

    def wait_for_connection(self, count=1):
        """Waits for a :class:`BluetoothMailboxClient` on a remote device to
        connect.

        Arguments:
            count (int):
                The number of remote connections to wait for.

        Raises:
            OSError:
                There was a problem establishing the connection.
        """
        for _ in range(count):
            self.handle_request()


class MailboxRFCOMMClient(ThreadingRFCOMMClient):
    def __init__(self, parent, bdaddr):
        self.parent = parent
        super().__init__((bdaddr, EV3_RFCOMM_CHANNEL), MailboxHandler)

    def send(self, data):
        self.socket.send(data)

    def finish_request(self, request, client_address):
        self.RequestHandlerClass(request, client_address, self.parent)


class BluetoothMailboxClient(MailboxHandlerMixIn):
    """Object that represents outgoing Bluetooth connections to one or more
    remote EV3s.

    The remote EV3s can either be running MicroPython or the standard EV3
    firmare.
    """

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def connect(self, brick):
        """Connects to a :class:`BluetoothMailboxServer` on another device.

        The remote device must be paired and waiting for a connection. See
        :meth:`BluetoothMailboxServer.wait_for_connection`.

        Arguments:
            brick (str):
                The name or address of the remote EV3 to connect to.

        Raises:
            TypeError:
                ``brick`` is not a string
            ValueError:
                There are no paired Bluetooth devices that match ``brick``
                or connection to ``brick`` already exists.
            OSError:
                There was a problem establishing the connection.
        """
        addr = resolve(brick)
        if addr is None:
            raise ValueError('no paired devices matching "{}"'.format(brick))
        client = MailboxRFCOMMClient(self, addr)
        if self._clients.setdefault(addr, client) is not client:
            raise ValueError("connection with this address already exists")
        try:
            client.handle_request()
        except:
            del self._clients[addr]
            raise

    def close(self):
        """Closes the connections."""
        for client in self._clients.values():
            client.client_close()
        self._clients.clear()
