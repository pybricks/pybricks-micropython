# SPDX-License-Identifier: GPL-2.0-only
# Copyright 2006 David Anderson <david.anderson@calixo.net>

import struct

from nxt import lowlevel

ATMEL_VENDOR_ID = 0x03EB
SAMBA_PRODUCT_ID = 0x6124
SAMBA_USB_INTERFACE = 1


class SambaOpenError(Exception):
    """An error occured while opening a connection to SAM-BA"""


def _command(code, address):
    return "%c%08X#" % (code, address)


def _command2(code, address, value):
    return "%c%08X,%08X#" % (code, address, value)


class SambaBrick(object):
    def __init__(self):
        self.usb = None

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass

    def open(self, timeout=None):
        self.usb = lowlevel.get_device(
            ATMEL_VENDOR_ID, SAMBA_PRODUCT_ID, timeout=timeout
        )
        if not self.usb:
            raise SambaOpenError("Could not find a SAM-BA brick to connect to")
        self.usb.open(SAMBA_USB_INTERFACE, detach_kernel_driver=True)

        # Initial SAM-BA handshake.
        self.usb.write("N#")
        res = self.usb.read(2)
        if res != "\n\r":
            raise SambaOpenError("Incorrect handshake response")

    def close(self):
        self.usb.close()
        self.usb = None

    def write_byte(self, address, byte):
        assert 0 <= byte <= 255
        self.usb.write(_command2("O", address, byte))

    def write_halfword(self, address, halfword):
        assert 0 <= halfword <= (2**16 - 1)
        self.usb.write(_command2("H", address, halfword))

    def write_word(self, address, word):
        assert 0 <= word <= (2**32 - 1)
        self.usb.write(_command2("W", address, word))

    def write_buffer(self, address, data):
        self.usb.write(_command2("S", address, len(data)))
        self.usb.write(data)

    def _read_common(self, code, address, size, struct_code):
        assert size in (1, 2, 4)
        self.usb.write(_command2(code, address, size))

        res = self.usb.read(size).encode()
        return struct.unpack("<%c" % struct_code, res)[0]

    def read_byte(self, address):
        return self._read_common("o", address, 1, "B")

    def read_halfword(self, address):
        return self._read_common("h", address, 2, "H")

    def read_word(self, address):
        return self._read_common("w", address, 4, "L")

    def read_buffer(self, address, len):
        self.usb.write(_command2("R", address, len))
        return self.usb.read(len)

    def jump(self, address):
        self.usb.write(_command("G", address))

    def version(self):
        self.usb.write("V#")
        return self.usb.read(4)
