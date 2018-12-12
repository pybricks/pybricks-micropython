# MIT License
#
# Copyright (c) 2017 David Lechner <david@lechnology.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import time
import _thread

from uctypes import addressof
from uctypes import sizeof
from uctypes import struct
from uctypes import ARRAY
from uctypes import UINT8
from uctypes import UINT16
from usocket import socket
from usocket import SOCK_STREAM

from uev3dev.util import debug_print

# stuff from bluetooth/bluetooth.h

_AF_BLUETOOTH = 31
_BTPROTO_RFCOMM = 3
_BDADDR_ANY = '00:00:00:00:00:00'

_sa_family_t = UINT16
_bd_addr_t = {
    'b': (ARRAY | 0, UINT8 | 6)
}

_sockaddr_rc = {
    'rc_family': _sa_family_t | 0,
    'rc_bdaddr': (2, _bd_addr_t),
    'rc_channel': UINT8 | 8,
}


def _str2ba(string, ba):
    """Convert string to Bluetooth address"""
    for i, v in enumerate(string.split(':')):
        ba.b[5-i] = int(v, 16)


def _ba2str(ba):
    """Convert Bluetooth address to string"""
    string = []
    for b in ba.b:
        string.append('{:02X}'.format(b))
    string.reverse()
    return ':'.join(string)


class BluetoothRemote():
    """Object that represents a Bluetooth connection to a remote EV3 brick.

    Parameters:
        name (str): The MAC address of the remote brick
    """
    def __init__(self, name):
        # TODO: Need a way to convert name to MAC address (maybe)
        self._sock = socket(_AF_BLUETOOTH, SOCK_STREAM, _BTPROTO_RFCOMM)

        addr_data = bytearray(sizeof(_sockaddr_rc))
        addr = struct(addressof(addr_data), _sockaddr_rc)

        addr.rc_family = _AF_BLUETOOTH
        _str2ba(name, addr.rc_bdaddr)
        addr.rc_channel = 1

        self._sock.connect(addr_data)

    def close(self):
        self._sock.close()


class BluetoothServer():
    """Object that enables incoming Bluetooth connections from an EV3"""
    def __init__(self):
        self._sock = socket(_AF_BLUETOOTH, SOCK_STREAM, _BTPROTO_RFCOMM)

        addr_data = bytearray(sizeof(_sockaddr_rc))
        addr = struct(addressof(addr_data), _sockaddr_rc)

        addr.rc_family = _AF_BLUETOOTH
        _str2ba(_BDADDR_ANY, addr.rc_bdaddr)
        addr.rc_channel = 1

        self._sock.bind(addr_data)

    def start(self):
        """Start the server"""
        self._sock.listen(1)
        # TODO: start a background thread that accepts client connections

    def _accept(self):
        client, addr_data = self._sock.accept()
        try:
            addr = struct(addressof(addr_data), _sockaddr_rc)
            debug_print('client', _ba2str(addr.rc_bdaddr))
            while True:
                data = client.recv(1)
                debug_print(data)
                # TODO: need a lms2012 bytecode interpreter
        finally:
            client.close()

    def close(self):
        self._sock.close()


class Messaging():
    def wait_update(self, title):
        # FIXME: implement messaging
        self._lock = _thread.allocate_lock()
        self._lock.acquire()
        # just wait forever for now
        self._lock.acquire()
