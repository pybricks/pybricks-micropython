#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Partial implementation of the Pybricks USB CDC-ACM serial protocol.

Enough to reboot the hub into update mode, so we can flash new firmware.
"""

import time
import serial

from .cobs import DELIMITER, pack, unpack

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


def send_frame(ser: serial.Serial, message: bytes) -> None:
    """Frames ``message`` with SPIKE-variant COBS and writes it."""
    ser.write(pack(message))
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
        if byte[0] == DELIMITER:
            if buffer:
                return unpack(bytes(buffer))
            # Empty delimiter (flush) — keep waiting.
        else:
            buffer.append(byte[0])


def reboot_to_update_mode_pybricks(ser: serial.Serial) -> bool:
    """Reboot a Pybricks hub into firmware update (DFU) mode via the serial protocol.

    Reads the firmware version from the hub to confirm it is running Pybricks,
    then sends the reboot-to-update-mode command.

    Returns the firmware version as ``(major, minor)`` on success, or ``None``
    if the hub did not respond as a Pybricks hub.

    Returns:
        True if Pybricks was detected and the reboot command was sent, False otherwise.
    """
    # Send a lone delimiter to resync the hub's COBS framing on connect.
    ser.write(bytes([DELIMITER]))
    ser.flush()

    # Request the firmware version string (GATT characteristic 0x2A26).
    send_read(ser, READ_CHARACTERISTIC_GATT, 0x2A26)
    frame = read_frame(ser)
    if frame is None or frame[0] != IN_EP_MSG_READ_REPLY:
        return False
    try:
        version_str = frame[4:].decode("utf-8")
        parts = version_str.split(".")
        fw_major = int(parts[0])
        if fw_major != 4:
            print("Unexpected Pybricks firmware version: " + version_str)
            return False
    except (ValueError, IndexError, UnicodeDecodeError):
        return False

    send_command(ser, bytes([PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE]))
    ser.close()
    return True
