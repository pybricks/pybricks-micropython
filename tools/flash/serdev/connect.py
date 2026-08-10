#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

import sys
import serial
from serial.tools import list_ports
from lwp3.bytecodes import HubKind

# Supported USB identifiers
LEGO_USB_VID = 0x0694
HUB_PIDS = {
    HubKind.TECHNIC_LARGE: (0x0009, 0x0010),
    HubKind.TECHNIC_SMALL: (0x000D,),
    HubKind.EV3: (0x0005,),
    HubKind.NXT: (0x0002,),
}


def get_serial_device(expected_hub: HubKind):
    """Returns the serial device path for a connected hub."""

    ports = []
    for port in list_ports.comports():
        if port.vid == LEGO_USB_VID and port.pid in HUB_PIDS[expected_hub]:
            print(f"Found hub on {port.device}")
            ports.append(port.device)

    # Nothing found, we'll skip auto-reboot.
    if len(ports) == 0:
        return None

    if len(ports) > 1:
        sys.exit("Multiple Pybricks hubs found. Make sure there is only one.")

    # Return opened port if available.
    try:
        return serial.Serial(ports[0], baudrate=115200, timeout=0.1)
    except serial.SerialException:
        sys.exit(f"Could not open serial port. Is Pybricks Code using it?")
