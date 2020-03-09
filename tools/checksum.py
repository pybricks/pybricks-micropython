#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

"""Calculate checksum of firmware file in the same way that the bootloader on
the movehub does."""

from __future__ import print_function

import argparse
import struct


def sum_complement(fw, max_size):
    """Calculates the checksum of a firmware file using the sum complement
    method of adding each 32-bit word and the returning the two's complement
    as the checksum.

    Parameters
    ----------
    fw : file
        The firmware file (a binary buffer - e.g. a file opened in 'rb' mode)
    max_size : int
        The maximum size of the firmware file.

    Returns
    -------
    int
        The correction needed to make the checksum of the file == 0.
    """
    checksum = 0
    size = 0

    while True:
        word = fw.read(4)
        if not word:
            break
        checksum += struct.unpack("I", word)[0]
        size += 4

    if size > max_size:
        raise ValueError("File is too large")

    for _ in range(size, max_size, 4):
        checksum += 0xFFFFFFFF

    checksum &= 0xFFFFFFFF
    correction = checksum and (1 << 32) - checksum or 0

    return correction


# thanks https://stackoverflow.com/a/33152544/1976323

_CRC_TABLE = (
    0x00000000,
    0x04C11DB7,
    0x09823B6E,
    0x0D4326D9,
    0x130476DC,
    0x17C56B6B,
    0x1A864DB2,
    0x1E475005,
    0x2608EDB8,
    0x22C9F00F,
    0x2F8AD6D6,
    0x2B4BCB61,
    0x350C9B64,
    0x31CD86D3,
    0x3C8EA00A,
    0x384FBDBD,
)


def _dword(value):
    return value & 0xFFFFFFFF


def _crc32_fast(crc, data):
    crc, data = _dword(crc), _dword(data)
    crc ^= data
    for _ in range(8):
        crc = _dword(crc << 4) ^ _CRC_TABLE[crc >> 28]
    return crc


def _crc32_fast_block(crc, buffer):
    for data in buffer:
        crc = _crc32_fast(crc, data)
    return crc


def crc32_checksum(fw, max_size):
    """Calculate the checksum of a firmware file using CRC-32 as implemented
    in STM32 microprocessors.

    Parameters
    ----------
    fw : file
        The firmware file (a binary buffer - e.g. a file opened in 'rb' mode)
    max_size : int
        The maximum size of the firmware file.

    Returns
    -------
    int
        The checksum
    """

    # remove the last 4 bytes that are the placeholder for the checksum
    fw = fw.read()[:-4]
    if len(fw) + 4 > max_size:
        raise ValueError("File is too large")

    if len(fw) & 3:
        raise ValueError("bytes_data length must be multiple of four")

    crc = 0xFFFFFFFF
    for index in range(0, len(fw), 4):
        data = int.from_bytes(fw[index : index + 4], "little")
        crc = _crc32_fast(crc, data)
    return crc


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compute checksum.")
    parser.add_argument(
        "checksum_type", metavar="type", choices=["xor", "crc32"], help="checksum type"
    )
    parser.add_argument("fw_file", type=argparse.FileType("rb"), help="firmware file name")
    parser.add_argument("max_size", type=int, help="max size of firmware file")

    args = parser.parse_args()

    if args.checksum_type == "xor":
        print(hex(sum_complement(args.fw_file, args.max_size)))
    elif args.checksum_type == "crc32":
        print(hex(crc32_checksum(args.fw_file, args.max_size)))
