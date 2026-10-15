# SPDX-License-Identifier: MIT
# Copyright (c) 2021 The Pybricks Authors

import asyncio
import logging

from bleak import BleakClient, BleakScanner
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData

from util import chunk

logger = logging.getLogger(__name__)


async def find_device(
    name: str | None = None,
    service: str = "",
    timeout: float = 10,
) -> BLEDevice:
    """Finds a BLE device that is currently advertising that matches the
    given parameters.

    Arguments:
        name:
            The device name. This can also be the Bluetooth address on non-Apple
            platforms or a UUID on Apple platforms. If ``name`` is ``None`` then
            it is not used as part of the matching criteria. The name matching
            is not case-sensitive.
        service:
            The service UUID that is advertized.
        timeout:
            How long to search before giving up.

    Returns:
        The first detected matching device.

    Raises:
        asyncio.TimeoutError:
            Device was not found within the timeout.
    """

    def match_uuid_and_name(device: BLEDevice, adv: AdvertisementData):
        if service not in adv.service_uuids:
            return False

        if adv.local_name is None:
            # have not received SCAN_RSP yet
            return False

        if (
            name is not None
            and adv.local_name != name
            and device.address.upper() != name.upper()
        ):
            # filtering by name but name does not match
            return False

        return True

    device = await BleakScanner.find_device_by_filter(
        match_uuid_and_name, timeout, service_uuids=[service]
    )

    if device is None:
        raise asyncio.TimeoutError

    return device


class BLEConnection:
    """Configure BLE, connect, send data, and handle receive events."""

    def __init__(self, char_rx_UUID, char_tx_UUID, max_data_size):
        """Initializes and configures connection settings.

        Arguments:
            char_rx_UUID (str):
                UUID for RX.
            char_rx_UUID (str):
                UUID for TX.
            max_data_size (int):
                Maximum number of bytes per write operation.

        """
        # Save given settings
        self.char_rx_UUID = char_rx_UUID
        self.char_tx_UUID = char_tx_UUID
        self.max_data_size = max_data_size
        self.connected = False

    def data_handler(self, sender, data):
        """Handles new incoming data.

        This is usually overridden by a mixin class.

        Arguments:
            sender (str):
                Sender uuid.
            data (bytes):
                Bytes to process.
        """
        logger.debug("DATA {0}".format(data))

    def disconnected_handler(self, client: BleakClient):
        """Handles disconnected event."""
        logger.debug("Disconnected.")
        self.connected = False

    async def connect(self, device: BLEDevice):
        """Connects to a BLE device.

        Arguments:
            device: Client device
        """

        print("Connecting to", device)
        self.client = BleakClient(device)
        await self.client.connect(disconnected_callback=self.disconnected_handler)
        await self.client.start_notify(self.char_tx_UUID, self.data_handler)
        print("Connected successfully!")
        self.connected = True

    async def disconnect(self):
        """Disconnects the client from the server."""
        await self.client.stop_notify(self.char_tx_UUID)
        if self.connected:
            logger.debug("Disconnecting...")
            await self.client.disconnect()

    async def write(self, data, with_response=False):
        """Write bytes to the server, split to chunks of maximum data size.

        Arguments:
            data (bytearray):
                Data to be sent to the server.
            with_response (bool):
                Write with or without response.
        """
        # Send the chunks one by one
        for c in chunk(data, self.max_data_size):
            logger.debug(
                "TX CHUNK: {0}, {1} response".format(
                    c, "with" if with_response else "without"
                )
            )
            await self.client.write_gatt_char(
                self.char_rx_UUID, bytearray(c), with_response
            )


class BLERequestsConnection(BLEConnection):
    """Sends messages and awaits replies of known length.

    This can be used for devices with known commands and known replies, such
    as some bootloaders to update firmware over the air.
    """

    def __init__(self, UUID):
        """Initialize the BLE Connection."""
        self.reply_ready = asyncio.Event()
        self.prepare_reply()

        super().__init__(UUID, UUID, 1024)

    def data_handler(self, sender, data):
        """Handles new incoming data and raise event when a new reply is ready.

        Arguments:
            sender (str):
                Sender uuid.
            data (bytes):
                Bytes to process.
        """
        logger.debug("DATA {0}".format(data))
        self.reply = data
        self.reply_ready.set()

    def prepare_reply(self):
        """Clears existing reply and wait event.

        This is usually called prior to the write operation, to ensure we
        receive some of the bytes while are still awaiting the sending process.
        """
        self.reply = None
        self.reply_ready.clear()

    async def wait_for_reply(self, timeout=None):
        """Awaits for given number of characters since prepare_reply.

        Arguments:
            timeout (float or None):
                Time out to await. Same as asyncio.wait_for.

        Returns:
            bytearray: The reply.

        Raises:
            asyncio.TimeoutError: Same as asyncio.wait_for.
        """
        # Await for the reply ready event to be raised.
        await asyncio.wait_for(self.reply_ready.wait(), timeout)

        # Return reply and clear internal buffer
        reply = self.reply
        self.prepare_reply()
        return reply
