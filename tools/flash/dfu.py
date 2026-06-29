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
from pathlib import Path

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

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
MBOOT_PATH = REPO_ROOT / "bricks" / "primehub_f4" / "mboot.bin"

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

        # Set by user.
        self.mcu_family: str | None = None

    def set_mcu_family(self, mcu: str) -> None:
        """Sets the MCU family for this device.

        This is used to determine the flash sector layout for erasing.
        """
        self.mcu_family = mcu

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

    def get_string(self, index: int | None) -> str:
        """
        Safely reads a USB string descriptor, returning ``"<none>"`` if
        unset or ``"<error>"`` if it cannot be read.
        """
        if not index:
            return "<none>"
        try:
            return usb.util.get_string(self._dev, index) or "<none>"
        except Exception:
            return "<error>"

    def get_serial(self) -> str:
        """Gets the device serial number string descriptor."""
        return self.get_string(self._dev.iSerialNumber)

    def get_flash_layout_str(self) -> str:
        """Gets the DFU internal flash layout descriptor string."""
        cfg = self._dev[0]
        intf = cfg[(0, 0)]
        return self.get_string(intf.iInterface)

    def get_alt_setting_strs(self) -> list[tuple[int, str]]:
        """Gets the ``iInterface`` string for every alternate setting."""
        return [
            (intf.bAlternateSetting, self.get_string(intf.iInterface))
            for intf in self._dev[0]
        ]

    def mass_erase(self) -> None:
        """Erases the entire device."""
        self._dev.ctrl_transfer(0x21, _DFU_DNLOAD, 0, _DFU_INTERFACE, b"\x41", _TIMEOUT)
        self._check_status("erase", _DFU_STATE_DFU_DOWNLOAD_BUSY)
        self._check_status("erase", _DFU_STATE_DFU_DOWNLOAD_IDLE)

    def page_erase(self, addr: int) -> None:
        """Erases the single flash sector/page that contains ``addr``."""
        buf = struct.pack("<BI", 0x41, addr)
        self._dev.ctrl_transfer(0x21, _DFU_DNLOAD, 0, _DFU_INTERFACE, buf, _TIMEOUT)
        self._check_status("erase", _DFU_STATE_DFU_DOWNLOAD_BUSY)
        self._check_status("erase", _DFU_STATE_DFU_DOWNLOAD_IDLE)

    def _sector_layout(self) -> list[tuple[int, int]]:
        """Returns the ``(address, size)`` of every flash sector for ``mcu``.

        The sector layout is a fixed property of the silicon, so we hardcode
        it per MCU family rather than parsing the (fragile, vendor-specific)
        DFU flash layout descriptor string. This is independent of which
        bootloader is installed.
        """
        if self.mcu_family == "f4":
            # Standard STM32F4 1 MB layout: 16/16/16/16/64/128*7 KB.
            sizes_kb = (16, 16, 16, 16, 64, 128, 128, 128, 128, 128, 128, 128)
            addr = FLASH_BASE_ADDRESS
            sectors = []
            for size_kb in sizes_kb:
                sectors.append((addr, size_kb * 1024))
                addr += size_kb * 1024
            return sectors

        if self.mcu_family == "h5":
            # STM32H5 uses uniform 8 KB sectors across its flash.
            # We have the 1 MB variant.
            page = 8 * 1024
            count = (1 * 1024 * 1024) // page
            return [(FLASH_BASE_ADDRESS + i * page, page) for i in range(count)]

        raise ValueError(f"Unknown MCU type: {self.mcu_family}")

    def region_erase(
        self,
        start: int,
        end: int | None = None,
        progress: Callable[[int], None] | None = None,
    ) -> None:
        """Erases every flash sector overlapping ``[start, end)``.

        The MCU family is read from ``self.mcu_family`` to determine the flash
        sector layout. If ``end`` is ``None``, erases everything from ``start``
        to the end of flash. Sectors are erased whole, so an erase may extend
        slightly beyond ``start``/``end`` to the enclosing sector boundaries.
        ``progress`` is called with the number of bytes erased after each
        sector.
        """
        sectors = self._sector_layout()
        flash_start = sectors[0][0]
        flash_end = sectors[-1][0] + sectors[-1][1]
        if end is None:
            end = flash_end
        if not (flash_start <= start < end <= flash_end):
            raise ValueError(
                f"erase range [{start:#x}, {end:#x}) is invalid or outside "
                f"flash [{flash_start:#x}, {flash_end:#x})"
            )
        for addr, size in sectors:
            if addr + size <= start or addr >= end:
                continue
            self.page_erase(addr)
            if progress:
                progress(size)

    def _region_erase_size(self, start: int, end: int) -> int:
        """Returns the total number of bytes the sectors overlapping
        ``[start, end)`` occupy (i.e. how many bytes ``region_erase`` clears).
        """
        return sum(
            size
            for addr, size in self._sector_layout()
            if not (addr + size <= start or addr >= end)
        )

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

    def write_firmware(self, data: bytes, address: int) -> None:
        """Erases the affected flash region and writes ``data`` at ``address``.

        This is the single low-level entry point: it makes no assumptions
        about what the bytes are, it just puts them on the device. Callers are
        responsible for validating the firmware before calling this. The
        sectors overlapping ``[address, address + len(data))`` are erased
        before writing.

        This does NOT leave DFU mode, so multiple regions can be written in
        the same session. Call :meth:`exit` once when all writes are done.
        """
        end = address + len(data)

        print("Erasing flash...")
        with (
            logging_redirect_tqdm(),
            tqdm(
                total=self._region_erase_size(address, end),
                unit="B",
                unit_scale=True,
            ) as pbar,
        ):
            self.region_erase(address, end, pbar.update)

        print("Writing new firmware...")
        with (
            logging_redirect_tqdm(),
            tqdm(total=len(data), unit="B", unit_scale=True) as pbar,
        ):
            self.write_memory(address, data, pbar.update)

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


def determine_platform(
    flash_layout: str, serial: str, vid: int, pid: int, bcd: int
) -> tuple[str, bool]:
    """Determines the hub and version from the USB device properties."""

    if vid != LEGO_USB_VID:
        sys.exit(f"This is not a LEGO hub. Unknown USB vendor ID: {vid:04X}")

    # There is only one variant with this hub, and we use the stock frozen
    # bootloader. We do not install a second-stage mboot.
    if pid == SPIKE_ESSENTIAL_DFU_USB_PID:
        return "f4", "essential_hub"

    # Otherwise only expect SPIKE Prime or MINDSTORMS Inventor. Either can come
    # in various variants, which we'll distinguish below.
    if pid != SPIKE_PRIME_DFU_USB_PID and pid != MINDSTORMS_INVENTOR_DFU_USB_PID:
        sys.exit(
            f"Did not detect Prime Hub, Inventor Hub, or Essential Hub. Unknown USB product ID: {pid:04X}"
        )

    # This is the frozen stock bootloader for either the Prime Hub or Inventor
    # Hub. This is always the F4. Allows legacy firmware or new firmware if we
    # install the second-stage bootloader.
    if bcd == 0x0100:
        return "f4", "prime_hub"

    # Now we may still have an F4 with the second stage bootloader already installed
    # or an H5 with its own frozen stock bootloader (modern mboot). Both advertise
    # the same bcdDevice, at this specific version.
    if bcd != 0x0300:
        sys.exit(f"Unknown bcdDevice: {bcd:04X}")

    # Since both variants have the same bcdDevice and product ID, we need to
    # distinguish them by their flash layout. Neither need a new mboot.
    if "064Kg" in flash_layout and "128Kg" in flash_layout:
        return "f4", "prime_hub_f4"

    if "08Kg" in flash_layout:
        return "h5", "prime_hub_h5"

    # No other known variants exist, so we can assume this is an unknown hub.
    sys.exit(f"Unknown MCU: {flash_layout}")


def flash_dfu(firmwares: dict[str, bytes], hub_kind: HubKind) -> None:
    """
    Flashes firmware to the hub using USB DFU.

    Args:
        firmwares: Mapping of platform name to raw firmware binary blob.
        hub_kind: The hub type ID. Only hubs matching this ID will be discovered.

    Raises:
        ValueError: If there is no firmware for the given hub kind.
    """
    devices = get_dfu_devices(idVendor=LEGO_USB_VID)
    if not devices:
        sys.exit(
            "No DFU devices found. "
            + "Make sure hub is in DFU mode and connected with USB."
        )
    if len(devices) > 1:
        sys.exit("Multiple DFU devices found. Connect at most one.")

    try:
        dfu = DfuDevice(devices[0])
    except USBError as e:
        if e.errno == errno.EACCES and platform.system() == "Linux":
            sys.exit(
                "Permission to access USB device denied. Did you install udev rules?"
            )
        else:
            raise  # not expecting other errors, so re-raise.

    # Figure out what is attached.
    mcu, attached_platform = determine_platform(
        flash_layout=dfu.get_flash_layout_str(),
        serial=dfu.get_serial(),
        vid=int(dfu._dev.idVendor),
        pid=int(dfu._dev.idProduct),
        bcd=int(dfu._dev.bcdDevice),
    )
    print(f"Attached platform: {attached_platform} with MCU: {mcu}")
    dfu.set_mcu_family(mcu)

    # There is only one viable path for Essential Hub, across all versions.
    if hub_kind == HubKind.TECHNIC_SMALL:
        if (
            attached_platform != "essential_hub"
            or len(firmwares) > 1
            or "essential_hub" not in firmwares
        ):
            sys.exit("Incompatible firmware for this device.")
        print("===Installing SPIKE Essential firmware.===")
        dfu.write_firmware(
            firmwares["essential_hub"], FLASH_BASE_ADDRESS + BOOTLOADER_SIZE_32K
        )
        dfu.exit()
        print("Done.")
        sys.exit(0)

    # Otherwise only expect SPIKE Prime or MINDSTORMS Inventor.
    if hub_kind != HubKind.TECHNIC_LARGE:
        sys.exit(f"Unsupported hub kind: {hub_kind}")

    # Backwards compatibility for v1.x and v2.x SPIKE Prime firmware archives
    # which did not specify pbio_platform. It is always F4 with stock bootloader.
    # So in this case the zip dictates what the device must be.
    if "prime_hub" in firmwares:
        print("Legacy firmware detected.")
        if attached_platform != "prime_hub" or len(firmwares) > 1:
            sys.exit("Incompatible firmware for this DFU mode.")
        print("===Installing legacy SPIKE Prime firmware.===")
        dfu.write_firmware(
            firmwares["prime_hub"], FLASH_BASE_ADDRESS + BOOTLOADER_SIZE_32K
        )
        dfu.exit()
        print("Done.")
        sys.exit(0)

    # There is only one path for the H5, directly after the frozen bootloader.
    # So in this case, the platform dictates what firmware must be included.
    if attached_platform == "prime_hub_h5":
        if "prime_hub_h5" not in firmwares:
            sys.exit("Incompatible firmware for this device.")
        firmware_h5 = firmwares["prime_hub_h5"]
        if firmware_h5[0x264] >= 0x0F:
            sys.exit("This firmware is not compatible with mboot.")
        print("===Installing SPIKE Prime H5 firmware.===")
        dfu.write_firmware(firmware_h5, FLASH_BASE_ADDRESS + BOOTLOADER_SIZE_64K)
        dfu.exit()
        print("Done.")
        sys.exit(0)

    # Now we know we have an F4, but it may be in either frozen bootloader. We
    # use the same firmware for both.
    if "prime_hub_f4" not in firmwares:
        sys.exit("Incompatible firmware for this device.")
    if firmwares["prime_hub_f4"][0x1F0] >= 0x0F:
        sys.exit("This firmware is not compatible with mboot.")

    # Decide whether we need the second-stage bootloader.
    if attached_platform == "prime_hub_f4":
        # We are running the second-stage bootloader already, so can directly
        # flash the firmware.
        firmware_f4 = firmwares["prime_hub_f4"]
        address = FLASH_BASE_ADDRESS + BOOTLOADER_SIZE_64K
    elif attached_platform == "prime_hub":
        # We are running the frozen bootloader, so we need to install the
        # second-stage bootloader first, then the firmware.
        print("Adding second-stage mboot bootloader.")
        with open(MBOOT_PATH, "rb") as f:
            mboot_bin = f.read()
            if len(mboot_bin) != BOOTLOADER_SIZE_32K:
                sys.exit(f"Unexpected mboot size: {len(mboot_bin)} bytes.")
        firmware_f4 = mboot_bin + firmwares["prime_hub_f4"]
        address = FLASH_BASE_ADDRESS + BOOTLOADER_SIZE_32K
    else:
        sys.exit(f"Unsupported attached platform: {attached_platform}")

    # Install the selected or combined firmware
    print("===Installing SPIKE Prime F4 firmware.===")
    dfu.write_firmware(firmware_f4, address)
    dfu.exit()
    print("Done.")
    sys.exit(0)
