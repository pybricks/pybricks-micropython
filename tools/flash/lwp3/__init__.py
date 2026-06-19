# SPDX-License-Identifier: MIT
# Copyright (c) 2021-2022 The Pybricks Authors

"""
This module and its submodules are used for Bluetooth Low Energy
communications with devices that provide the LEGO Wireless Protocol v3.
"""

from lwp3.bytecodes import Capabilities, HubKind, LastNetwork, Status

# LEGO Wireless Protocol v3 is defined at:
# https://lego.github.io/lego-ble-wireless-protocol-docs/


LEGO_CID = 0x0397
"""LEGO System A/S company identifier.

This number is assigned by the Bluetooth SIG.

https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/
"""


def _lwp3_uuid(short: int) -> str:
    """Get a 128-bit UUID from a ``short`` UUID.

    Args:
        short: The 16-bit UUID.

    Returns:
        The 128-bit UUID as a string.
    """
    return f"0000{short:04x}-1212-efde-1623-785feabcd123"


LWP3_HUB_SERVICE_UUID = _lwp3_uuid(0x1623)
"""LEGO wireless protocol v3 hub service UUID."""

LWP3_HUB_CHARACTERISTIC_UUID = _lwp3_uuid(0x1624)
"""LEGO wireless protocol v3 hub characteristic UUID."""

LWP3_BOOTLOADER_SERVICE_UUID = _lwp3_uuid(0x1625)
"""LEGO wireless protocol v3 bootloader service UUID."""

LWP3_BOOTLOADER_CHARACTERISTIC_UUID = _lwp3_uuid(0x1626)
"""LEGO wireless protocol v3 bootloader characteristic UUID."""


class AdvertisementData:
    """
    The 6-byte manufacturer-specific advertisement data sent when a hub running
    official LEGO firmware is advertising.
    """

    def __init__(self, data: bytes) -> None:
        if len(data) != 6:
            raise ValueError("expecting 6 bytes of data")

        self._data = data

    def __bytes__(self) -> bytes:
        return self._data

    @property
    def is_button_pressed(self) -> bool:
        """
        Gets the state of the button on the hub.
        """
        return bool(self._data[0])

    @property
    def hub_kind(self) -> HubKind:
        """
        Gets the hub type identifier.
        """
        return HubKind(self._data[1])

    @property
    def hub_capabilities(self) -> Capabilities:
        """
        Gets the hub capability flags.
        """
        return Capabilities(self._data[2])

    @property
    def last_network(self) -> LastNetwork:
        """
        Gets the ID of the last network the hub was connected to.
        """
        return LastNetwork(self._data[3])

    @property
    def status(self) -> Status:
        """
        Gets the status of the advertising hub.
        """
        return Status(self._data[4])
