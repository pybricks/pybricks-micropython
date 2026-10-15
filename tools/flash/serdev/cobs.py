#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Consistent Overhead Byte Stuffing (COBS) framing, SPIKE Prime variant.

This is the same scheme implemented in the firmware (see lib/pbio/src/cobs.c
and pbio/cobs.h). Both the SPIKE Prime protocol (spike.py) and the Pybricks USB
protocol (pybricks.py) frame their messages with this encoding.
"""

DELIMITER = 0x02
NO_DELIMITER = 0xFF
COBS_CODE_OFFSET = DELIMITER
MAX_BLOCK_SIZE = 84
XOR = 0x03


def cobs_encode(data: bytes) -> bytearray:
    """Encode data with SPIKE Prime's COBS variant (no 0x00/0x01/0x02 in output)."""
    buffer = bytearray()
    code_index = block = 0

    def begin_block():
        nonlocal code_index, block
        code_index = len(buffer)
        buffer.append(NO_DELIMITER)
        block = 1

    begin_block()
    for byte in data:
        if byte > DELIMITER:
            buffer.append(byte)
            block += 1
        if byte <= DELIMITER or block > MAX_BLOCK_SIZE:
            if byte <= DELIMITER:
                delimiter_base = byte * MAX_BLOCK_SIZE
                block_offset = block + COBS_CODE_OFFSET
                buffer[code_index] = delimiter_base + block_offset
            begin_block()
    buffer[code_index] = block + COBS_CODE_OFFSET
    return buffer


def cobs_decode(data: bytes) -> bytes:
    """Decode data using SPIKE Prime's COBS variant."""
    buffer = bytearray()

    def unescape(code: int):
        if code == NO_DELIMITER:
            return None, MAX_BLOCK_SIZE + 1
        value, block = divmod(code - COBS_CODE_OFFSET, MAX_BLOCK_SIZE)
        if block == 0:
            block = MAX_BLOCK_SIZE
            value -= 1
        return value, block

    if not data:
        return b""
    value, block = unescape(data[0])
    for byte in data[1:]:
        block -= 1
        if block > 0:
            buffer.append(byte)
            continue
        if value is not None:
            buffer.append(value)
        value, block = unescape(byte)
    return bytes(buffer)


def pack(data: bytes) -> bytes:
    """COBS-encode, XOR with 0x03, and frame with a trailing 0x02 delimiter."""
    buffer = cobs_encode(data)
    for i in range(len(buffer)):
        buffer[i] ^= XOR
    buffer.append(DELIMITER)
    return bytes(buffer)


def unpack(frame: bytes) -> bytes:
    """Unframe (strip optional 0x01 prefix), un-XOR, COBS-decode."""
    start = 1 if frame and frame[0] == 0x01 else 0
    unframed = bytes(b ^ XOR for b in frame[start:])
    return cobs_decode(unframed)
