# SPDX-License-Identifier: MIT
# Copyright (C) 2020 David Lechner

from ustruct import pack, unpack


class Mailbox:
    def __init__(self, name, connection, encode=repr, decode=eval):
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
        self._encode = encode
        self._decode = decode

    def read(self):
        """Reads the current value of the mailbox.

        Returns:
            The decoded value or ``None`` if the mailbox has never received
            a value.
        """
        data = self._connection.read_from_mailbox(self.name)
        return None if data is None else self._decode(data)

    def send(self, value, destination=None):
        """Sends a value to remote mailboxes with the same name as this
        mailbox.

        Arguments:
            value: The value to send.
            destination: The name or address of a specific device or ``None``
                to broadcast to all connected devices.
        """
        data = self._encode(value)
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


def _encode_logic(value):
    return b'\x01' if value else b'\x00'


def _decode_logic(payload):
    return bool(payload[0])


class EV3LogicMailbox(Mailbox):
    def __init__(self, name, connection):
        """:class:`Mailbox` that holds a logic or boolean value.

        This is compatible with the "logic" message blocks in the standard
        EV3 firmware.

        Arguments:
            name (str):
                The name of this mailbox.
            connection:
                A connection object that implements the mailbox connection
                interface.
        """
        super().__init__(name, connection, _encode_logic, _decode_logic)


def _encode_numeric(value):
    return pack('<f', value)


def _decode_numeric(payload):
    return unpack('<f', payload)[0]


class EV3NumericMailbox(Mailbox):
    def __init__(self, name, connection):
        """:class:`Mailbox` that holds a numeric or floating point value.

        This is compatible with the "numeric" message blocks in the standard
        EV3 firmware.

        Arguments:
            name (str):
                The name of this mailbox.
            connection:
                A connection object that implements the mailbox connection
                interface.
        """
        super().__init__(name, connection, _encode_numeric, _decode_numeric)


def _encode_text(value):
    return '{}\0'.format(value)


def _decode_text(payload):
    return payload.decode().strip('\0')


class EV3TextMailbox(Mailbox):
    def __init__(self, name, connection):
        """:class:`Text` that holds a text or string point value.

        This is compatible with the "text" message blocks in the standard
        EV3 firmware.

        Arguments:
            name (str):
                The name of this mailbox.
            connection:
                A connection object that implements the mailbox connection
                interface.
        """
        super().__init__(name, connection, _encode_text, _decode_text)
