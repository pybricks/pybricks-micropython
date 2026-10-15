# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors

"""
Helper functions for calculating checksums.
"""

from io import BytesIO


def xor_bytes(data: bytes, init: int = 0xFF) -> int:
    """
    Computes a checksum by xoring each byte in ``data`` starting with ``init``.

    Args:
        data: The byte data to perform the checksum on.
        init: The initial value of the checksum.

    Returns:
        The calculated checksum.
    """
    checksum = init

    for b in data:
        checksum ^= b

    return checksum


def sum_complement(data: BytesIO, max_size: int) -> int:
    """
    Calculates the checksum of using the sum complement method of adding each
    32-bit word (little-endian) and the returning the two's complement as the
    checksum.

    Arguments:
        data:
            A binary buffer - e.g. a file opened in 'rb' mode.
        max_size:
            The maximum size of the firmware file.

    Returns:
        The correction needed to make the checksum == 0.
    """
    checksum = 0
    size = 0

    while True:
        word = data.read(4)
        if not word:
            break
        checksum += int.from_bytes(word, "little")
        size += 4

    if size + 4 > max_size:
        raise ValueError("data is too large")

    for _ in range(size, max_size - 4, 4):
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


def _dword(value: int) -> int:
    return value & 0xFFFFFFFF


def _crc32_fast(crc, data: int) -> int:
    crc, data = _dword(crc), _dword(data)
    crc ^= data
    for _ in range(8):
        crc = _dword(crc << 4) ^ _CRC_TABLE[crc >> 28]
    return crc


def crc32_checksum(data: BytesIO, max_size: int) -> int:
    """
    Calculate the checksum using CRC-32 as implemented in STM32 microprocessors.

    Args:
        data:
            A binary buffer - e.g. a file opened in 'rb' mode.
        max_size:
            The maximum size of the firmware file.

    Returns:
        The checksum.
    """

    data = data.read()

    if len(data) + 4 > max_size:
        raise ValueError("data is too large")

    if len(data) & 3:
        raise ValueError("bytes_data length must be multiple of four")

    crc = 0xFFFFFFFF
    for index in range(0, len(data), 4):
        word = int.from_bytes(data[index : index + 4], "little")
        crc = _crc32_fast(crc, word)
    return crc
