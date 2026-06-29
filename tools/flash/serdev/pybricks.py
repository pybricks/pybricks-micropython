#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Partial implementation of the Pybricks USB CDC-ACM serial protocol.

Enough to reboot the hub into update mode, so we can flash new firmware.
"""

import time
import serial

# Host -> hub command types (see pbio_pybricks_command_t in protocol.h).
PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE = 5

# Host -> hub message types (see pbio_pybricks_usb_out_ep_msg_t in protocol.h).
OUT_EP_MSG_COMMAND = 2
OUT_EP_MSG_READ = 3

# Hub -> host message types (see pbio_pybricks_usb_in_ep_msg_t in protocol.h).
IN_EP_MSG_READ_REPLY = 3

# Characteristic namespace for read requests
# (see PBIO_PYBRICKS_USB_INTERFACE_READ_CHARACTERISTIC_* in protocol.h).
READ_CHARACTERISTIC_GATT = 0x01

COBS_DELIMITER = 0x00


def cobs_encode(data: bytes) -> bytes:
    """COBS-encodes ``data``. The result never contains a zero byte."""
    out = bytearray()
    code_idx = len(out)
    out.append(0)  # placeholder for the first code byte
    code = 1

    for byte in data:
        if byte == 0:
            out[code_idx] = code
            code_idx = len(out)
            out.append(0)
            code = 1
        else:
            out.append(byte)
            code += 1
            if code == 0xFF:
                out[code_idx] = code
                code_idx = len(out)
                out.append(0)
                code = 1

    out[code_idx] = code
    return bytes(out)


def cobs_decode(data: bytes) -> bytes:
    """COBS-decodes a single frame (delimiter already stripped)."""
    out = bytearray()
    idx = 0
    n = len(data)

    while idx < n:
        code = data[idx]
        idx += 1
        for _ in range(1, code):
            if idx >= n:
                return b""  # malformed
            out.append(data[idx])
            idx += 1
        if code < 0xFF and idx < n:
            out.append(0)

    return bytes(out)


def send_frame(ser: serial.Serial, message: bytes) -> None:
    """COBS-encodes ``message`` and writes it as a delimited frame."""
    ser.write(cobs_encode(message) + bytes([COBS_DELIMITER]))
    ser.flush()


def send_command(ser: serial.Serial, payload: bytes) -> None:
    """Sends a Pybricks command (prefixed with the OUT command discriminator)."""
    send_frame(ser, bytes([OUT_EP_MSG_COMMAND]) + payload)


def send_read(ser: serial.Serial, service: int, char_id: int) -> None:
    """Sends a characteristic read request; the hub replies with a READ_REPLY."""
    send_frame(
        ser, bytes([OUT_EP_MSG_READ, service, char_id & 0xFF, (char_id >> 8) & 0xFF])
    )


def read_frame(ser: serial.Serial, timeout: float = 0.5) -> bytes | None:
    """Reads bytes until a COBS delimiter is received or ``timeout`` expires.

    Returns the decoded frame payload, or ``None`` on timeout.
    """
    buffer = bytearray()
    deadline = time.monotonic() + timeout
    ser.timeout = max(0.05, timeout)

    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return None
        ser.timeout = remaining
        byte = ser.read(1)
        if not byte:
            # Timed out
            return None
        if byte[0] == COBS_DELIMITER:
            if buffer:
                return cobs_decode(bytes(buffer))
            # Empty delimiter (flush) — keep waiting.
        else:
            buffer.append(byte[0])


def reboot_to_update_mode_pybricks(ser: serial.Serial) -> None:
    """Reboot a Pybricks hub into firmware update (DFU) mode via the serial protocol.

    Reads the firmware version from the hub to confirm it is running Pybricks,
    then sends the reboot-to-update-mode command.

    Returns the firmware version as ``(major, minor)`` on success, or ``None``
    if the hub did not respond as a Pybricks hub.
    """
    # Send a lone delimiter to resync the hub's COBS framing on connect.
    ser.write(bytes([COBS_DELIMITER]))
    ser.flush()

    # At this point, combined with poking at the SPIKE Prime firmware before
    # we have sent to the hub: b'\x02' + b'\x00\x00\x02' + b'\x00'. Which splits
    # to only two single byte messages, both: b'\x02' which are discarded by
    # the pybricks firmware, so won't get it into a bad state.

    # Request the firmware version string (GATT characteristic 0x2A26).
    send_read(ser, READ_CHARACTERISTIC_GATT, 0x2A26)
    frame = read_frame(ser)
    if frame is None or frame[0] != IN_EP_MSG_READ_REPLY:
        return
    try:
        version_str = frame[4:].decode("utf-8")
        parts = version_str.split(".")
        fw_major = int(parts[0])
        if fw_major != 4:
            print("Unexpected Pybricks firmware version: " + version_str)
            return
    except (ValueError, IndexError, UnicodeDecodeError):
        return

    send_command(ser, bytes([PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE]))
    ser.close()
