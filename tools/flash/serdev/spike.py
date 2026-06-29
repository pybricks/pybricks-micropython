#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Partial implementation of the SPIKE Prime serial protocol.
https://lego.github.io/spike-prime-docs/

Enough to reboot the hub into update mode, so we can flash new firmware.
"""

import struct
import time
from dataclasses import dataclass

import serial

DELIMITER = 0x02
NO_DELIMITER = 0xFF
COBS_CODE_OFFSET = DELIMITER
MAX_BLOCK_SIZE = 84
XOR = 0x03

MSG_NAMES = {
    0x00: "InfoRequest",
    0x01: "InfoResponse",
}


@dataclass
class InfoResponse:
    rpc_major: int
    rpc_minor: int
    rpc_build: int
    fw_major: int
    fw_minor: int
    fw_build: int
    max_packet: int
    max_message: int
    max_chunk: int
    device_type: int


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


def unpack(frame: bytes) -> bytes:
    """Unframe (strip optional 0x01 prefix), un-XOR, COBS-decode."""
    start = 1 if frame and frame[0] == 0x01 else 0
    unframed = bytes(b ^ XOR for b in frame[start:])
    return cobs_decode(unframed)


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


def pack(data: bytes) -> bytes:
    """COBS-encode, XOR with 0x03, and frame with a trailing 0x02 delimiter."""
    buffer = cobs_encode(data)
    for i in range(len(buffer)):
        buffer[i] ^= XOR
    buffer.append(DELIMITER)
    return bytes(buffer)


def decode_info_response(msg: bytes) -> InfoResponse:
    if len(msg) < 17 or msg[0] != 0x01:
        raise ValueError("Invalid InfoResponse message")
    """Decode a 0x01 InfoResponse payload (all little-endian)."""
    return InfoResponse(*struct.unpack("<BBHBBHHHHH", msg[1:17]))


def print_info_response(info: InfoResponse) -> None:
    """Pretty-print a decoded InfoResponse."""
    print(
        f" - {'RPC version':<26} {info.rpc_major}.{info.rpc_minor} build {info.rpc_build}"
    )
    print(
        f" - {'Firmware version':<26} {info.fw_major}.{info.fw_minor} build {info.fw_build}"
    )
    print(f" - {'Max packet size':<26} {info.max_packet}")
    print(f" - {'Max message size':<26} {info.max_message}")
    print(f" - {'Max chunk size':<26} {info.max_chunk}")
    print(f" - {'Product group device type':<26} {info.device_type}")


def send_message(ser: serial.Serial, payload: bytes) -> None:
    """Frame a protocol message and write it to the hub, echoing it locally."""
    ser.write(pack(payload))


def read_frame(ser: serial.Serial, timeout: float) -> bytes | None:
    """Reads bytes until a SPIKE frame delimiter (0x02) is received or ``timeout`` expires.

    Returns the decoded message payload, or ``None`` on timeout.
    """
    buffer = bytearray()
    deadline = time.monotonic() + timeout

    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return None
        ser.timeout = remaining
        byte = ser.read(1)
        if not byte:
            return None  # timed out
        if byte[0] == DELIMITER:
            if buffer:
                return unpack(bytes(buffer))
            # Empty delimiter — keep waiting.
        else:
            buffer.append(byte[0])


def reboot_for_update_spike_prime(ser: serial.Serial) -> bool:
    """Reboot a SPIKE Prime hub into firmware update (DFU) mode via the serial protocol.

    Sends an InfoRequest to confirm the hub is running official SPIKE Prime
    firmware, then triggers the MicroPython bootloader via the REPL.

    Returns True on success or False if the hub did not respond as a SPIKE Prime
    hub with official firmware.
    """
    # The hub needs a short moment after the port opens before it will
    # accept/answer the first message.
    time.sleep(0.1)

    # Send a lone delimiter to resync the hub's COBS framing on connect.
    ser.write(bytes([DELIMITER]))
    ser.flush()

    # Ask the hub to identify itself.
    send_message(ser, bytes([0x00]))  # InfoRequest
    msg = read_frame(ser, timeout=0.5)
    if not msg or msg[0] != 0x01:
        return False

    info = decode_info_response(msg)
    print_info_response(info)

    if (info.fw_major, info.fw_minor) < (1, 8):
        print(
            f"SPIKE Prime firmware {info.fw_major}.{info.fw_minor} is too old to reboot to update mode."
        )
        return False

    # Send Ctrl-C three times to interrupt any running program and drop to REPL.
    ser.write(b"\x03\x03\x03")
    ser.flush()

    # Wait for the REPL prompt.
    deadline = time.monotonic() + 3.0
    buf = b""
    while b">>> " not in buf:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            print("[repl] timed out waiting for prompt")
            break
        ser.timeout = remaining
        byte = ser.read(1)
        if not byte:
            break  # timed out
        buf += byte + ser.read(ser.in_waiting)

    # There is no RPC protocol command for this, but we can enter DFU mode
    # via MicroPython.
    ser.write(b"import machine; machine.bootloader()\r\n")
    ser.flush()
    ser.close()

    return True
