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

import uctypes

from fcntl import ioctl

# from linux/i2c.h

_I2C_M_TEN = 0x0010
_I2C_M_RD = 0x0001
_I2C_M_STOP = 0x8000
_I2C_M_NOSTART = 0x4000
_I2C_M_REV_DIR_ADDR = 0x2000
_I2C_M_IGNORE_NAK = 0x1000
_I2C_M_NO_RD_ACK = 0x0800
_I2C_M_RECV_LEN = 0x0400

_I2C_FUNC_I2C = 0x00000001
_I2C_FUNC_10BIT_ADDR = 0x00000002
_I2C_FUNC_PROTOCOL_MANGLING = 0x00000004
_I2C_FUNC_SMBUS_PEC = 0x00000008
_I2C_FUNC_NOSTART = 0x00000010
_I2C_FUNC_SLAVE = 0x00000020
_I2C_FUNC_SMBUS_BLOCK_PROC_CALL = 0x00008000
_I2C_FUNC_SMBUS_QUICK = 0x00010000
_I2C_FUNC_SMBUS_READ_BYTE = 0x00020000
_I2C_FUNC_SMBUS_WRITE_BYTE = 0x00040000
_I2C_FUNC_SMBUS_READ_BYTE_DATA = 0x00080000
_I2C_FUNC_SMBUS_WRITE_BYTE_DATA = 0x00100000
_I2C_FUNC_SMBUS_READ_WORD_DATA = 0x00200000
_I2C_FUNC_SMBUS_WRITE_WORD_DATA = 0x00400000
_I2C_FUNC_SMBUS_PROC_CALL = 0x00800000
_I2C_FUNC_SMBUS_READ_BLOCK_DATA = 0x01000000
_I2C_FUNC_SMBUS_WRITE_BLOCK_DATA = 0x02000000
_I2C_FUNC_SMBUS_READ_I2C_BLOCK = 0x04000000
_I2C_FUNC_SMBUS_WRITE_I2C_BLOCK = 0x08000000

_I2C_SMBUS_BLOCK_MAX = 32

_i2c_smbus_data = {
    'byte': uctypes.UINT8 | 0,
    'word': uctypes.UINT16 | 0,
    'block': (uctypes.ARRAY | 0, uctypes.UINT8 | (_I2C_SMBUS_BLOCK_MAX + 2))
}

_size_of_i2c_smbus_data = uctypes.sizeof(_i2c_smbus_data)

_I2C_SMBUS_READ = 1
_I2C_SMBUS_WRITE = 0

_I2C_SMBUS_QUICK = 0
_I2C_SMBUS_BYTE = 1
_I2C_SMBUS_BYTE_DATA = 2
_I2C_SMBUS_WORD_DATA = 3
_I2C_SMBUS_PROC_CALL = 4
_I2C_SMBUS_BLOCK_DATA = 5
_I2C_SMBUS_I2C_BLOCK_BROKEN = 6
_I2C_SMBUS_BLOCK_PROC_CALL = 7
_I2C_SMBUS_I2C_BLOCK_DATA = 8

# from linux/i2c-dev.h

_I2C_RETRIES = 0x0701
_I2C_TIMEOUT = 0x0702
_I2C_SLAVE = 0x0703
_I2C_SLAVE_FORCE = 0x0706
_I2C_TENBIT = 0x0704
_I2C_FUNCS = 0x0705
_I2C_RDWR = 0x0707
_I2C_PEC = 0x0708
_I2C_SMBUS = 0x0720

_i2c_smbus_ioctl_data = {
    'read_write': uctypes.UINT8 | 0,
    'command': uctypes.UINT8 | 1,
    'size': uctypes.UINT32 | 4,
    'data': uctypes.PTR | 8
}

_size_of_i2c_smbus_ioctl_data = uctypes.sizeof(_i2c_smbus_ioctl_data)


class SMBus():
    """Micropython implementation of SMBus"""

    _slave = 0

    def __init__(self, path):
        """Create a new SMBus instance
        :param string path: The path to the I2C device node, e.g. ``/dev/i2c0``.
        """
        self._devnode = open(path, 'w+')
        self._fd = self._devnode.fileno()
        flags = bytes(4)
        ioctl(self._fd, _I2C_FUNCS, flags, mut=True)
        flags = uctypes.struct(uctypes.addressof(flags), {
            'flags': uctypes.UINT32  # unsigned long
        }).flags
        self._func = {
            'i2c': bool(flags & _I2C_FUNC_I2C),
            'ten_bit_addr': bool(flags & _I2C_FUNC_10BIT_ADDR),
            'protocol_mangling': bool(flags & _I2C_FUNC_PROTOCOL_MANGLING),
            'smbus_pec': bool(flags & _I2C_FUNC_SMBUS_PEC),
            'no_start': bool(flags & _I2C_FUNC_NOSTART),
            'slave': bool(flags & _I2C_FUNC_SLAVE),
            'smbus_block_proc_call': bool(flags &
                                          _I2C_FUNC_SMBUS_BLOCK_PROC_CALL),
            'smbus_quick': bool(flags & _I2C_FUNC_SMBUS_QUICK),
            'smbus_read_byte': bool(flags & _I2C_FUNC_SMBUS_READ_BYTE),
            'smbus_write_byte': bool(flags & _I2C_FUNC_SMBUS_WRITE_BYTE),
            'smbus_write_data': bool(flags & _I2C_FUNC_SMBUS_WRITE_BYTE_DATA),
            'smbus_read_word_data': bool(flags &
                                         _I2C_FUNC_SMBUS_READ_WORD_DATA),
            'smbus_write_word_data': bool(flags &
                                          _I2C_FUNC_SMBUS_WRITE_WORD_DATA),
            'smbus_proc_call': bool(flags & _I2C_FUNC_SMBUS_PROC_CALL),
            'smbus_read_block_data': bool(flags &
                                          _I2C_FUNC_SMBUS_READ_BLOCK_DATA),
            'smbus_read_i2c_block': bool(flags &
                                         _I2C_FUNC_SMBUS_READ_I2C_BLOCK),
            'smbus_write_i2c_block': bool(flags &
                                          _I2C_FUNC_SMBUS_WRITE_I2C_BLOCK),
        }

    def set_address(self, address):
        ioctl(self._fd, _I2C_SLAVE, address)

    def _access(self, read_write, command, size, data):
        b = bytearray(_size_of_i2c_smbus_ioctl_data)
        args = uctypes.struct(uctypes.addressof(b), _i2c_smbus_ioctl_data)
        args.read_write = read_write
        args.command = command
        args.size = size
        args.data = uctypes.addressof(data)
        ioctl(self._fd, _I2C_SMBUS, args, mut=True)

    def write_quick(self, value):
        self._access(value, 0, _I2C_SMBUS_QUICK, None)

    def read_byte(self):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        self._access(_I2C_SMBUS_READ, 0, _I2C_SMBUS_BYTE, data)
        return data.byte

    def write_byte(self, value):
        self._access(_I2C_SMBUS_WRITE, value, _I2C_SMBUS_BYTE, None)

    def read_byte_data(self, command):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        self._access(_I2C_SMBUS_READ, command, _I2C_SMBUS_BYTE_DATA, data)
        return data.byte

    def write_byte_data(self, command, value):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        data.byte = value
        self._access(_I2C_SMBUS_WRITE, command, _I2C_SMBUS_BYTE_DATA, data)

    def read_word_data(self, command):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        self._access(_I2C_SMBUS_READ, command, _I2C_SMBUS_WORD_DATA, data)
        return data.word

    def write_word_data(self, command, value):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        data.word = value
        self._access(_I2C_SMBUS_WRITE, command, _I2C_SMBUS_WORD_DATA, data)

    def process_call(self, command, value):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        data.word = value
        self._access(_I2C_SMBUS_WRITE, command, _I2C_SMBUS_PROC_CALL, data)
        return data.word

    def read_block_data(self, command, values):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        self._access(_I2C_SMBUS_READ, command, _I2C_SMBUS_BLOCK_DATA, data)
        return data.block

    def write_block_data(self, command, values):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        values = values[:32]
        data.block = [len(values)] + values
        self._access(_I2C_SMBUS_WRITE, command, _I2C_SMBUS_BLOCK_DATA, data)

    def read_i2c_block_data(self, command, length):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        length = min(length, 32)
        data.block[0] = length
        if length == 32:
            size = _I2C_SMBUS_I2C_BLOCK_BROKEN
        else:
            size = _I2C_SMBUS_I2C_BLOCK_DATA
        self._access(_I2C_SMBUS_READ, command, size, data)
        return data.block[1:][:data.block[0]]

    def write_i2c_block_data(self, command, values):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        values = values[:32]
        data.block = [len(values)] + values
        self._access(_I2C_SMBUS_WRITE, command, _I2C_SMBUS_I2C_BLOCK_BROKEN,
                     data)

    def block_process_call(self, command, values):
        b = bytearray(_size_of_i2c_smbus_data)
        data = uctypes.struct(uctypes.addressof(b), _i2c_smbus_data)
        values = values[:32]
        data.block = [len(values)] + values
        self._access(_I2C_SMBUS_WRITE, command, _I2C_SMBUS_BLOCK_PROC_CALL,
                     data)
        return data.block[1:][:data.block[0]]
