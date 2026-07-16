# SPDX-License-Identifier: GPL-2.0-only
# Copyright 2006 David Anderson <david.anderson@calixo.net>

import struct
import time

import serial
from serial.tools import list_ports

ATMEL_VENDOR_ID = 0x03EB
SAMBA_PRODUCT_ID = 0x6124


class SambaOpenError(Exception):
    """An error occurred while opening a connection to SAM-BA"""


def _command(code, address):
    return f"{code}{address:08X}#".encode("ascii")


def _command2(code, address, value):
    return f"{code}{address:08X},{value:08X}#".encode("ascii")


class SambaBrick:
    def __init__(self):
        self.ser = None

    def open(self, timeout=5):
        # enumerate serial ports
        matches = [
            p
            for p in list_ports.comports()
            if p.vid == ATMEL_VENDOR_ID and p.pid == SAMBA_PRODUCT_ID
        ]

        if not matches:
            raise SambaOpenError("No SAM-BA device found (03eb:6124).")

        if len(matches) > 1:
            raise SambaOpenError(
                "Multiple SAM-BA devices found; cannot choose automatically."
            )

        port = matches[0].device

        try:
            self.ser = serial.Serial(
                port=port,
                timeout=timeout,
                write_timeout=timeout,
                exclusive=True,
            )
        except serial.SerialException as e:
            raise SambaOpenError(str(e))

        # Give CDC ACM time to settle
        time.sleep(0.1)

        self.ser.reset_input_buffer()
        self.ser.reset_output_buffer()

        # Initial SAM-BA handshake.
        self._write(b"N#")
        res = self._read_exact(2)

        if res != b"\n\r":
            raise SambaOpenError(f"Incorrect handshake response: {res!r}")

    def close(self):
        if self.ser:
            self.ser.close()
            self.ser = None

    def _write(self, data: bytes):
        self.ser.write(data)
        self.ser.flush()

    def _read_exact(self, n: int) -> bytes:
        buf = bytearray()
        while len(buf) < n:
            chunk = self.ser.read(n - len(buf))
            if not chunk:
                raise SambaOpenError("Timeout while reading from SAM-BA")
            buf.extend(chunk)
        return bytes(buf)

    def write_byte(self, address, byte):
        assert 0 <= byte <= 0xFF
        self._write(_command2("O", address, byte))

    def write_halfword(self, address, halfword):
        assert 0 <= halfword <= 0xFFFF
        self._write(_command2("H", address, halfword))

    def write_word(self, address, word):
        assert 0 <= word <= 0xFFFFFFFF
        self._write(_command2("W", address, word))

    def write_buffer(self, address, data):
        self._write(_command2("S", address, len(data)))
        self._write(data)

    def _read_common(self, code, address, size, struct_code):
        assert size in (1, 2, 4)
        self._write(_command2(code, address, size))

        raw = self._read_exact(size)
        return struct.unpack("<" + struct_code, raw)[0]

    def read_byte(self, address):
        return self._read_common("o", address, 1, "B")

    def read_halfword(self, address):
        return self._read_common("h", address, 2, "H")

    def read_word(self, address):
        return self._read_common("w", address, 4, "I")

    def read_buffer(self, address, len):
        self._write(_command2("R", address, len))
        return self._read_exact(len)

    def jump(self, address):
        self._write(_command("G", address))

    def version(self):
        self._write(b"V#")
        return self._read_exact(4)
