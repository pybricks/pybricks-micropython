# SPDX-License-Identifier: MIT
# Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
# Copyright (c) 2019-2026 The Pybricks Authors

# The USB DFU protocol implementation in this file was originally copied from
# the OpenMV project (micropython/tools/pydfu.py) and is licensed under the MIT
# license. Rewritten to skip the DFU format roundtrip.

"""Flashes firmware to a LEGO hub over USB using the STM32 DFU protocol."""

import errno
import platform
import struct
import sys
from collections.abc import Callable
from typing import NamedTuple

import usb.core
import usb.util
from usb.core import USBError
from tqdm.auto import tqdm
from tqdm.contrib.logging import logging_redirect_tqdm

from lwp3.bytecodes import HubKind
from constants import (
    LEGO_USB_VID,
    MINDSTORMS_INVENTOR_DFU_USB_PID,
    SPIKE_ESSENTIAL_DFU_USB_PID,
    SPIKE_PRIME_DFU_USB_PID,
)

BOOTLOADER_SIZE_32K = 32 * 1024
BOOTLOADER_SIZE_64K = 64 * 1024
FLASH_BASE_ADDRESS = 0x08000000
FLASH_SIZE = 1 * 1024 * 1024

# Maps each known DFU USB product ID to the hub it belongs to.
ALL_PIDS = {
    MINDSTORMS_INVENTOR_DFU_USB_PID: HubKind.TECHNIC_LARGE,
    SPIKE_ESSENTIAL_DFU_USB_PID: HubKind.TECHNIC_SMALL,
    SPIKE_PRIME_DFU_USB_PID: HubKind.TECHNIC_LARGE,
}
ALL_DEVICES = [f"{LEGO_USB_VID:04x}:{pid:04x}" for pid in ALL_PIDS.keys()]


# ---------------------------------------------------------------------------
# Low-level STM32 DFU protocol
# ---------------------------------------------------------------------------

# USB control transfer timeout in milliseconds.
_TIMEOUT = 4000

# DFU class requests (AN3156).
_DFU_DNLOAD = 1
_DFU_GETSTATUS = 3
_DFU_CLRSTATUS = 4
_DFU_ABORT = 6

# DFU states returned by GETSTATUS.
_DFU_STATE_DFU_IDLE = 0x02
_DFU_STATE_DFU_DOWNLOAD_BUSY = 0x04
_DFU_STATE_DFU_DOWNLOAD_IDLE = 0x05
_DFU_STATE_DFU_MANIFEST = 0x07
_DFU_STATE_DFU_UPLOAD_IDLE = 0x09

_DFU_DESCRIPTOR_TYPE = 0x21
_DFU_INTERFACE = 0


class CfgDescriptor(NamedTuple):
    bLength: int
    bDescriptorType: int
    bmAttributes: int
    wDetachTimeOut: int
    wTransferSize: int
    bcdDFUVersion: int


CFG_DESCRIPTOR_FORMAT = "<BBBHHH"


def find_dfu_cfg_descriptor(descr: bytes) -> CfgDescriptor | None:
    if len(descr) == 9 and descr[0] == 9 and descr[1] == _DFU_DESCRIPTOR_TYPE:
        return CfgDescriptor(*struct.unpack(CFG_DESCRIPTOR_FORMAT, bytearray(descr)))
    return None


def get_dfu_devices(**kwargs) -> list[usb.core.Device]:
    """Returns the list of connected USB devices that are in DFU mode."""

    def is_dfu_device(device: usb.core.Device) -> bool:
        """Returns True if the USB device is in DFU mode."""
        for cfg in device:
            for intf in cfg:
                return intf.bInterfaceClass == 0xFE and intf.bInterfaceSubClass == 1
        return False

    return list(usb.core.find(find_all=True, custom_match=is_dfu_device, **kwargs))


class DfuDevice:
    """A handle to a single STM32 device in DFU mode."""

    def __init__(self, device: usb.core.Device) -> None:
        self._dev = device
        device.set_configuration()
        usb.util.claim_interface(device, _DFU_INTERFACE)

        # Find the DFU configuration descriptor to learn the transfer size.
        self._cfg_descr = None
        for cfg in device.configurations():
            self._cfg_descr = find_dfu_cfg_descriptor(cfg.extra_descriptors)
            if self._cfg_descr:
                break
            for itf in cfg.interfaces():
                self._cfg_descr = find_dfu_cfg_descriptor(itf.extra_descriptors)
                if self._cfg_descr:
                    break

        # Get the device into a known idle state.
        for _ in range(4):
            status = self._get_status()
            if status == _DFU_STATE_DFU_IDLE:
                break
            elif status in (
                _DFU_STATE_DFU_DOWNLOAD_IDLE,
                _DFU_STATE_DFU_UPLOAD_IDLE,
            ):
                self._dev.ctrl_transfer(
                    0x21, _DFU_ABORT, 0, _DFU_INTERFACE, None, _TIMEOUT
                )
            else:
                self._dev.ctrl_transfer(
                    0x21, _DFU_CLRSTATUS, 0, _DFU_INTERFACE, None, _TIMEOUT
                )

    def _get_status(self) -> int:
        stat = self._dev.ctrl_transfer(
            0xA1, _DFU_GETSTATUS, 0, _DFU_INTERFACE, 6, 20000
        )
        # Firmware can provide an optional string for any error.
        if stat[5]:
            message = usb.util.get_string(self._dev, stat[5])
            if message:
                print(message)
        return stat[4]

    def _check_status(self, stage: str, expected: int) -> None:
        status = self._get_status()
        if status != expected:
            raise SystemExit(f"DFU: {stage} failed (state {status})")

    def mass_erase(self) -> None:
        """Erases the entire device."""
        self._dev.ctrl_transfer(0x21, _DFU_DNLOAD, 0, _DFU_INTERFACE, b"\x41", _TIMEOUT)
        self._check_status("erase", _DFU_STATE_DFU_DOWNLOAD_BUSY)
        self._check_status("erase", _DFU_STATE_DFU_DOWNLOAD_IDLE)

    def _set_address(self, addr: int) -> None:
        buf = struct.pack("<BI", 0x21, addr)
        self._dev.ctrl_transfer(0x21, _DFU_DNLOAD, 0, _DFU_INTERFACE, buf, _TIMEOUT)
        self._check_status("set address", _DFU_STATE_DFU_DOWNLOAD_BUSY)
        self._check_status("set address", _DFU_STATE_DFU_DOWNLOAD_IDLE)

    def write_memory(
        self,
        addr: int,
        data: bytes,
        progress: Callable[[int], None] | None = None,
    ) -> None:
        """Writes ``data`` to flash starting at ``addr``.

        Assumes the target memory has already been erased. ``progress`` is
        called with the number of bytes written after each chunk.
        """
        total = len(data)
        written = 0
        while written < total:
            self._set_address(addr + written)

            chunk = min(self._cfg_descr.wTransferSize, total - written)
            self._dev.ctrl_transfer(
                0x21,
                _DFU_DNLOAD,
                2,
                _DFU_INTERFACE,
                data[written : written + chunk],
                _TIMEOUT,
            )
            self._check_status("write memory", _DFU_STATE_DFU_DOWNLOAD_BUSY)
            self._check_status("write memory", _DFU_STATE_DFU_DOWNLOAD_IDLE)

            written += chunk
            if progress:
                progress(chunk)

    def exit(self) -> None:
        """Exits DFU mode and starts running the firmware."""
        self._set_address(FLASH_BASE_ADDRESS)
        # Zero-length download triggers the manifestation/reset.
        self._dev.ctrl_transfer(0x21, _DFU_DNLOAD, 0, _DFU_INTERFACE, None, _TIMEOUT)
        try:
            if self._get_status() != _DFU_STATE_DFU_MANIFEST:
                print("Failed to reset device")
            usb.util.dispose_resources(self._dev)
        except OSError:
            pass


# ---------------------------------------------------------------------------
# High-level firmware flashing
# ---------------------------------------------------------------------------


def _get_bootloader_size(pid: int, bcd_device: int | None) -> int:
    """Gets the bootloader size for the connected DFU device."""
    # New hardware revision of SPIKE Prime released in 2026 has a larger bootloader.
    if pid == SPIKE_PRIME_DFU_USB_PID and bcd_device == 0x0300:
        return BOOTLOADER_SIZE_64K
    return BOOTLOADER_SIZE_32K


def write_firmware(
    device: usb.core.Device,
    data: bytes,
    address: int,
) -> None:
    """Mass-erases the device and writes ``data`` to flash at ``address``.

    This is the single low-level entry point: it makes no assumptions about
    what the bytes are, it just puts them on the device. Callers are
    responsible for validating the firmware before calling this.
    """
    dfu = DfuDevice(device)
    print("Erasing flash...")
    dfu.mass_erase()
    print("Writing new firmware...")
    with (
        logging_redirect_tqdm(),
        tqdm(total=len(data), unit="B", unit_scale=True) as pbar,
    ):
        dfu.write_memory(address, data, pbar.update)
    dfu.exit()
    print("Done.")


def flash_dfu(firmware_bin: bytes, hub_kind: HubKind) -> None:
    """Flashes a firmware image to a connected hub over USB DFU."""
    try:
        devices = get_dfu_devices(idVendor=LEGO_USB_VID)
        if not devices:
            print(
                "No DFU devices found.",
                "Make sure hub is in DFU mode and connected with USB.",
                file=sys.stderr,
            )
            exit(1)

        if len(devices) > 1:
            print(
                "Multiple DFU devices found. Connect at most one.",
                file=sys.stderr,
            )
            exit(1)

        device = devices[0]
        product_id = int(device.idProduct)
        bcd_device = int(device.bcdDevice)

        if product_id not in ALL_PIDS:
            print(f"Unknown USB product ID: {product_id:04X}", file=sys.stderr)
            exit(1)

        if ALL_PIDS[product_id] != hub_kind:
            print("Incorrect firmware type for this hub", file=sys.stderr)
            exit(1)

        bootloader_size = _get_bootloader_size(product_id, bcd_device)
        firmware_address = FLASH_BASE_ADDRESS + bootloader_size

        write_firmware(device, firmware_bin, firmware_address)
    except USBError as e:
        if e.errno != errno.EACCES or platform.system() != "Linux":
            # not expecting other errors
            raise

        print(
            "Permission to access USB device denied. Did you install udev rules?",
            file=sys.stderr,
        )
        print(
            "Run `pybricksdev udev | sudo tee /etc/udev/rules.d/99-pybricksdev.rules` then try again.",
            file=sys.stderr,
        )
        exit(1)
