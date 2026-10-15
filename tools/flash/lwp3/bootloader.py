# SPDX-License-Identifier: MIT
# Copyright (c) 2021-2022 The Pybricks Authors

"""
The bootloader module contains the bytecodes and messages for using the separate
LWP3 bootloader service and characteristic for flashing firmware on the hub.
"""

from enum import IntEnum

from lwp3.bytecodes import Capabilities, HubKind, Version

# Bootloader characteristic bytecodes


class BootloaderCommand(IntEnum):
    """Commands that are sent to the bootloader GATT characteristic."""

    ERASE_FLASH = 0x11
    """Erases the flash memory."""

    PROGRAM_FLASH = 0x22
    """Writes to a segment of the flash memory."""

    START_APP = 0x33
    """Starts running the firmware (causes Bluetooth to disconnect)."""

    INIT_LOADER = 0x44
    """Initializes the firmware flasher."""

    GET_INFO = 0x55
    """Gets info about the hub and flash memory layout."""

    GET_CHECKSUM = 0x66
    """Gets the current checksum for the data that has been written so far."""

    GET_FLASH_STATE = 0x77
    """Gets the STM32 flash memory debug protection state.

    Not all bootloaders support this command.
    """

    DISCONNECT = 0x88
    """Causes the remote device to disconnect from Bluetooth."""


class BootloaderMessageKind(IntEnum):
    """Type for messages received from bootlaoder GATT characteristic notifications.

    Messages that are a response to a command will have the same value as
    :class:`BootloaderCommand` instead of a value from this enum.
    """

    ERROR = 0x05
    """Indicates that an error occurred."""


class BootloaderResult(IntEnum):
    """Status returned by certain bootloader commands."""

    OK = 0x00
    """The command was successful."""

    ERROR = 0xFF
    """The command failed."""


class BootloaderError(IntEnum):
    """Error type returned by bootloader error messages."""

    UNKNOWN_COMMAND = 0x05
    """The command was not recognized."""


class BootloaderAdvertisementData:
    """
    The 6-byte manufacturer-specific advertisement data sent when a hub in
    bootloader mode is advertising.
    """

    def __init__(self, data: bytes) -> None:
        if len(data) != 6:
            raise ValueError("expecting 6 bytes of data")

        self._data = data

    def __bytes__(self) -> bytes:
        return self._data

    @property
    def version(self) -> Version:
        """
        Gets the bootloader software version.
        """
        return Version(int.from_bytes(self._data[:4], "little"))

    @property
    def hub_kind(self) -> HubKind:
        """
        Gets the hub type identifier.
        """
        return HubKind(self._data[4])

    @property
    def hub_capabilities(self) -> Capabilities:
        """
        Gets the hub capability flags.
        """
        return Capabilities(self._data[5])
