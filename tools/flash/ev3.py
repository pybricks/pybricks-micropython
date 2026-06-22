# SPDX-License-Identifier: MIT
# Copyright (c) 2023 The Pybricks Authors

import enum
import itertools
import struct
from typing import Callable

import hid

from util import chunk
from constants import EV3_BOOTLOADER_USB_PID, LEGO_USB_VID

import zlib
from tqdm.auto import tqdm
from tqdm.contrib.logging import logging_redirect_tqdm


class MessageType(enum.IntEnum):
    SYSTEM_COMMAND_REPLY = 0x01
    SYSTEM_COMMAND_NO_REPLY = 0x81
    SYSTEM_REPLY = 0x03
    SYSTEM_REPLY_ERROR = 0x05


class ReplyStatusCode(enum.IntEnum):
    SUCCESS = 0x00
    UNKNOWN_HANDLE = 0x01
    HANDLE_NOT_READY = 0x02
    CORRUPT_FILE = 0x03
    NO_HANDLES_AVAILABLE = 0x04
    NO_PERMISSION = 0x05
    ILLEGAL_PATH = 0x06
    FILE_EXISTS = 0x07
    END_OF_FILE = 0x08
    SIZE_ERROR = 0x09
    UNKNOWN_ERROR = 0x0A
    ILLEGAL_FILENAME = 0x0B
    ILLEGAL_CONNECTION = 0x0C


class Command(enum.IntEnum):
    BEGIN_DOWNLOAD_WITH_ERASE = 0xF0
    BEGIN_DOWNLOAD = 0xF1
    DOWNLOAD_DATA = 0xF2
    CHIP_ERASE = 0xF3
    START_APP = 0xF4
    GET_CHECKSUM = 0xF5
    GET_VERSION = 0xF6


class ReplyError(Exception):
    def __init__(self, status: ReplyStatusCode):
        super().__init__(status.name, status.value)


class EV3Bootloader:
    """
    Connection to LEGO MINDSTORMS EV3 bootloader for flashing firmware.
    """

    _MAX_DATA_SIZE = 1018
    """Max number of bytes that can be written at one time."""

    def __init__(self):
        self._device = hid.device()
        self._msg_count = itertools.count()

    def open(self) -> None:
        """
        Opens an HID connection to the EV3 bootloader.
        """
        self._device.open(vendor_id=LEGO_USB_VID, product_id=EV3_BOOTLOADER_USB_PID)

    def close(self) -> None:
        """
        Closes the underlying HID connection.
        """
        self._device.close()

    def _send_command(self, command: Command, payload: bytes | None = None) -> int:
        length = 4

        if payload is not None:
            if len(payload) > self._MAX_DATA_SIZE:
                raise ValueError("payload is too large")

            length += len(payload)

        # report_id is not used by the EV3 but is required by HIDAPI on Windows.
        # It does no harm on Linux, so we include it unconditionally. Note that
        # the report ID is automatically stripped from incoming messages by
        # HIDAPI on all platforms.
        report_id = 0

        message_number = next(self._msg_count)

        message = struct.pack(
            "<BHHBB",
            report_id,
            length,
            message_number,
            MessageType.SYSTEM_COMMAND_REPLY,
            command,
        )

        if payload is not None:
            message += payload

        self._device.write(message)

        return message_number

    def _receive_reply(
        self, command: Command, message_number: int, force_length: int = 0
    ) -> bytes:
        """
        Receive a reply from the EV3 bootloader.

        Args:
            command: The command that was sent.
            message_number: The return value of :meth:`_send_command`.
            force_length: Expected length, used only when it fails to unpack
                          normally. Some replies on USB 3.0 hosts contain
                          the original command written over the reply. This
                          means the header is bad, but the payload may be in
                          tact if you know what data to expect.

        Returns:
            The payload of the reply.
        """
        reply = bytes(self._device.read(255))
        length, reply_number, message_type, reply_command, status = struct.unpack_from(
            "<HHBBB", reply
        )

        if reply_number != message_number:
            raise RuntimeError(
                f"message sequence number mismatch: {reply_number} != {message_number}"
            )

        if message_type == MessageType.SYSTEM_REPLY_ERROR:
            raise ReplyError(status)

        if message_type != MessageType.SYSTEM_REPLY:
            if force_length:
                return reply[7 : force_length + 2]
            raise RuntimeError(f"unexpected message type: {message_type}")

        if reply_command != command:
            raise RuntimeError(f"command mismatch: {reply_command} != {command}")

        return reply[7 : length + 2]

    def download(
        self,
        data: bytes,
        progress: Callable[[int], None] | None = None,
    ) -> None:
        """
        Downloads a firmware blob to the EV3.

        This operation takes about 60 seconds for a full 16MB firmware file.

        Args:
            data: The data to write.
            progress: Optional callback for indicating progress.
        """

        completed = 0
        for c in chunk(data, self._MAX_DATA_SIZE):
            num = self._send_command(Command.DOWNLOAD_DATA, c)
            try:
                completed += len(c)
                self._receive_reply(Command.DOWNLOAD_DATA, num)
            except RuntimeError as e:
                # Allow exception only on the final chunk.
                if completed != len(data):
                    raise e
                print(e, ". Proceeding anyway.")

            if progress:
                progress(len(c))

    def erase_and_begin_download(self, size) -> None:
        """
        Erases the external flash memory chip by the amount required to
        flash the new firmware. Also prepares firmware download.

        Args:
            size: How much to erase.
        """
        param_data = struct.pack("<II", 0, size)
        num = self._send_command(Command.BEGIN_DOWNLOAD_WITH_ERASE, param_data)
        self._receive_reply(Command.BEGIN_DOWNLOAD_WITH_ERASE, num)

    def start_app(self) -> None:
        """
        Starts the app from external flash memory.
        """
        num = self._send_command(Command.START_APP)
        self._receive_reply(Command.START_APP, num)

    def get_checksum(self, address: int, size: int) -> int:
        """
        Gets the checksum of a memory range.

        Args:
            address: The starting address.
            size: The size of data in bytes used to compute the checksum.

        Returns:
            The checksum.
        """
        payload = struct.pack("<II", address, size)
        num = self._send_command(Command.GET_CHECKSUM, payload)
        payload = self._receive_reply(Command.GET_CHECKSUM, num)
        return struct.unpack("<I", payload)[0]

    def get_version(self) -> tuple[int, int]:
        """
        Gets the bootloader firmware version and the hardware version.

        Returns:
            Tuple containing the firmware and hardware versions.
        """
        num = self._send_command(Command.GET_VERSION)
        # On certain USB 3.0 systems, the brick reply contains the command
        # we just sent written over it. This means we don't get the correct
        # header and length info. Since the command here is smaller than the
        # reply, the paypload does not get overwritten, so we can still get
        # the version info since we know the expected reply size.
        payload = self._receive_reply(Command.GET_VERSION, num, force_length=13)
        return struct.unpack("<II", payload)

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, *args):
        self.close()

    def __del__(self) -> None:
        self.close()


def flash_ev3(firmware: bytes | dict[str, bytes]) -> None:
    """Flashes firmware to EV3."""

    # There is only one firmware for the EV3. It deals with hardware
    # variations like Bluetooth chipsets at runtime.
    if isinstance(firmware, dict):
        firmware = firmware["ev3"]

    # TODO: nice error message and exit(1) if EV3 is not found
    with EV3Bootloader() as bootloader:
        fw, hw = bootloader.get_version()
        print(f"hwid: {hw}")

        print("Erasing memory and preparing firmware download...")
        bootloader.erase_and_begin_download(len(firmware))

        print("Downloading firmware...")
        with (
            logging_redirect_tqdm(),
            tqdm(total=len(firmware), unit="B", unit_scale=True) as pbar,
        ):
            bootloader.download(firmware, pbar.update)

        print("Verifying...", end="", flush=True)
        checksum = bootloader.get_checksum(0, len(firmware))
        expected_checksum = zlib.crc32(firmware)

        if checksum != expected_checksum:
            print("Bad checksum!")
            exit(1)

        print("OK.")

        print("Restarting EV3...", end="", flush=True)
        bootloader.start_app()
        print("Done.")
