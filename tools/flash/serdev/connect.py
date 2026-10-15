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


def get_hub_name(port) -> str | None:
    """Gets the hub name from a port's USB product string.

    The firmware advertises itself as "<name> (<hub type>)", see
    pbsys_host_get_hub_display_name(), so the name is everything before the
    type. Taking it from the descriptor means hubs can be told apart without
    opening them, so a hub that something else is talking to is still
    recognizable.
    """
    product = port.product
    if not product:
        return None

    name, separator, _ = product.rpartition(" (")
    return name if separator else product


def get_serial_device(expected_hub: HubKind, name: str | None = None):
    """Returns the serial device for a connected hub.

    Args:
        expected_hub: The kind of hub to look for.
        name: Which hub to use when several are attached. Ignored when only one
            is, so that renaming the hub in front of you always works. A hub
            that is already in its bootloader does not appear here at all, so
            it is flashed whatever it used to be called.
    """

    ports = [
        port
        for port in list_ports.comports()
        if port.vid == LEGO_USB_VID and port.pid in HUB_PIDS[expected_hub]
    ]

    for port in ports:
        print(f"Found hub '{get_hub_name(port)}' on {port.device}")

    # Nothing found, we'll skip auto-reboot.
    if len(ports) == 0:
        return None

    if len(ports) > 1:
        if not name:
            sys.exit(
                "Multiple Pybricks hubs found. Pass NAME=<hub name> to pick "
                "one, or leave only one attached."
            )

        matches = [port for port in ports if get_hub_name(port) == name]

        if not matches:
            found = ", ".join(f"{p.device} ({get_hub_name(p)})" for p in ports)
            sys.exit(f"No attached hub is called '{name}'. Found: {found}.")

        if len(matches) > 1:
            found = ", ".join(p.device for p in matches)
            sys.exit(f"Several attached hubs are called '{name}': {found}.")

        ports = matches

    # Return opened port if available.
    try:
        return serial.Serial(ports[0].device, baudrate=115200, timeout=0.1)
    except serial.SerialException:
        sys.exit(f"Could not open {ports[0].device}. Is Pybricks Code or pb using it?")
