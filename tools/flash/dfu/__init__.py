# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2021 The Pybricks Authors

import errno
import os
import platform
import sys
from tempfile import TemporaryDirectory

from dfu import dfu_create
from usb.core import USBError

from dfu import dfu_upload
from lwp3.bytecodes import HubKind
from firmware import AnyFirmwareMetadata
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


ALL_PIDS = {
    MINDSTORMS_INVENTOR_DFU_USB_PID: HubKind.TECHNIC_LARGE,
    SPIKE_ESSENTIAL_DFU_USB_PID: HubKind.TECHNIC_SMALL,
    SPIKE_PRIME_DFU_USB_PID: HubKind.TECHNIC_LARGE,
}
ALL_DEVICES = [f"{LEGO_USB_VID:04x}:{pid:04x}" for pid in ALL_PIDS.keys()]


def _get_bootloader_size(pid: int, bcd_device: int | None) -> int:
    """Gets bootloader size for the connected DFU device."""
    # New hardware revision of SPIKE Prime released in 2026 has a larger bootloader.
    if pid == SPIKE_PRIME_DFU_USB_PID and bcd_device == 0x0300:
        return BOOTLOADER_SIZE_64K

    return BOOTLOADER_SIZE_32K


def _get_firmware_region(bootloader_size: int) -> tuple[int, int]:
    """Gets firmware flash address and size from bootloader size."""
    return FLASH_BASE_ADDRESS + bootloader_size, FLASH_SIZE - bootloader_size


def flash_dfu(firmware_bin: bytes, metadata: AnyFirmwareMetadata) -> None:
    """Flashes a firmware file using DFU."""

    with TemporaryDirectory() as out_dir:
        outfile = os.path.join(out_dir, "firmware.dfu")

        try:
            # Determine correct product ID

            devices = dfu_upload.get_dfu_devices(idVendor=LEGO_USB_VID)
            if not devices:
                print(
                    "No DFU devices found.",
                    "Make sure hub is in DFU mode and connected with USB.",
                    file=sys.stderr,
                )
                exit(1)

            product_id = int(devices[0].idProduct)
            bcd_device = int(devices[0].bcdDevice)
            if product_id not in ALL_PIDS:
                print(f"Unknown USB product ID: {product_id:04X}", file=sys.stderr)
                exit(1)

            if ALL_PIDS[product_id] != metadata["device-id"]:
                print("Incorrect firmware type for this hub", file=sys.stderr)
                exit(1)

            bootloader_size = _get_bootloader_size(product_id, bcd_device)
            firmware_address, _ = _get_firmware_region(bootloader_size)
            target: dfu_create.Image = {
                "address": firmware_address,
                "data": firmware_bin,
            }

            # Create dfu file
            device = "0x{0:04x}:0x{1:04x}".format(LEGO_USB_VID, product_id)
            dfu_create.build(outfile, [[target]], device)

            # Init dfu tool
            dfu_upload.__VID = LEGO_USB_VID
            dfu_upload.__PID = product_id
            dfu_upload.init()
            elements = dfu_upload.read_dfu_file(outfile)
            assert elements is not None

            # Erase flash
            print("Erasing flash...")
            dfu_upload.mass_erase()

            # Upload dfu file
            print("Writing new firmware...")
            dfu_upload.write_elements(elements, True, dfu_upload.cli_progress)
            dfu_upload.exit_dfu()
            print("Done.")
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
