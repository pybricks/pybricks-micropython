# SPDX-License-Identifier: GPL-2.0-only
# Copyright 2006 David Anderson <david.anderson@calixo.net>
# Copyright 2023 The Pybricks Authors

import struct


class Error(Exception):
    """Exception base class for this program."""


class FileTooSmall(Error):
    """File is too small to be a firmware."""


class FileTooLarge(Error):
    """File is too large to fit in flash memory."""


class InvalidHeader(Error):
    """Firmware has invalid header."""


class Firmware:
    FLASH_SIZE = 256 * 1024  # NXT has 256 KiB flash memory
    MIN_SIZE = 500  # needs at least a vector table and some init code
    HEADER_DEF = "<5L?"
    HEADER_SIZE = struct.calcsize(HEADER_DEF)

    def __init__(self, fw_and_header: bytes):
        if len(fw_and_header) < Firmware.HEADER_SIZE + Firmware.MIN_SIZE:
            raise FileTooSmall("firmware is too small to be a valid firmware.")

        header = fw_and_header[-self.HEADER_SIZE :]
        self.firmware = fw_and_header[: -self.HEADER_SIZE]

        (
            magic,
            self.ramsize,
            self.romsize,
            self.writeaddr,
            self.loadaddr,
            self.samba,
        ) = struct.unpack(self.HEADER_DEF, header)

        if magic != 0xDEADBEEF:
            raise InvalidHeader("Bad magic on header")

        if len(self.firmware) > Firmware.FLASH_SIZE:
            raise FileTooLarge(
                f"provided firmware is {len(self.firmware)} bytes but flash memory is only {Firmware.FLASH_SIZE} bytes"
            )
