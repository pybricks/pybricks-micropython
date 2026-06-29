#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

import sys
import serial
from serial.tools import list_ports

# Supported USB identifiers
LEGO_USB_VID = 0x0694
HUB_PIDS = {
    0x0009: "SPIKE Prime",
    0x000D: "SPIKE Essential",
    0x0010: "MINDSTORMS Robot Inventor",
    0x0005: "MINDSTORMS EV3",
    0x0002: "MINDSTORMS NXT",
}


def get_serial_device():
    """Returns the serial device path for a connected hub."""

    ports = []
    for port in list_ports.comports():
        if port.vid == LEGO_USB_VID and port.pid in HUB_PIDS:
            print(f"Found {HUB_PIDS[port.pid]} on {port.device}")
            ports.append(port.device)

    # Nothing found, we'll skip auto-reboot.
    if len(ports) == 0:
        return None

    if len(ports) > 1:
        sys.exit("Multiple Pybricks hubs found. Make sure there is only one.")

    # Return the serial port for the single hub found.
    return serial.Serial(ports[0], baudrate=115200, timeout=0.1)
