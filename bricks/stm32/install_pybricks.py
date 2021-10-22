# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2021 The Pybricks Authors
#
# This is a Pybricks firmware installer for:
# - SPIKE Prime Hub
# - SPIKE Essential Hub
# - MINDSTORMS Robot Inventor Hub
#
# Manual installation instructions:
# - Ensure that the hub is powered via USB.
# - Upload the contents of firmware.zip to _firmware/ on the hub.
# - Run this script. If you use the REPL, you can do it this way:
#
#    from _firmware.install_pybricks import install
#    install()
#
# - On success, the hub reboots and installs the Pybricks firmware.
#
# You can also use Pybricksdev or Pybricks Code, which will do this for you.


import firmware

import ubinascii
import uhashlib
import ujson
import umachine

import sys


# File paths
FIRMWARE_DIR = "/_firmware"
PYBRICKS_FIRMWARE_PATH = FIRMWARE_DIR + "/firmware.bin"
PYBRICKS_METADATA_PATH = FIRMWARE_DIR + "/firmware.metadata.json"
LEGO_FIRMWARE_PATH = FIRMWARE_DIR + "/lego-firmware.bin"
LEGO_METADATA_PATH = FIRMWARE_DIR + "/lego-firmware.metadata.json"

# Error messages.
ERROR_BAD_FIRMWARE = """Invalid firmware file."""
ERROR_BAD_HUB = """Incorrect hub type."""
ERROR_DUAL_BOOT = """You are running dual-boot. Please re-install the official firmware first."""
ERROR_EXTERNAL_FLASH = """Unable to create space for Pybricks firmware."""
ERROR_FIRMWARE_COPY_FAILED = """Unable to copy the Pybricks firmware."""

# Constants.
FLASH_FIRMWARE_START = 0x8008000
FLASH_READ_SIZE = 32
FLASH_WRITE_SIZE = FLASH_READ_SIZE * 4
HUB_IDS = {
    "LEGO Technic Large Hub(0x0009)": 0x81,
    "LEGO Technic Large Hub(0x0010)": 0x81,
    "LEGO Technic Small Hub(0x000D)": 0x83,
}


def read_internal_flash(address, length):
    """Read a given number of bytes from a given absolute address."""
    return firmware.flash_read(address - FLASH_FIRMWARE_START)[0:length]


def read_internal_flash_int(address):
    """Gets a little endian uint32 integer from the internal flash."""
    return int.from_bytes(read_internal_flash(address, 4), "little")


def stop_installation(reason):
    """Stops the installation and explains why."""
    print(reason)
    print("Pybricks will not be installed.")
    sys.exit(1)


def get_lego_firmware_info():
    """Gets information about the running firmware."""

    # Get firmware/device ID
    hub_id = firmware.id_string()
    if hub_id not in HUB_IDS:
        stop_installation(ERROR_BAD_HUB)

    # Get firmware reset vector
    firmware_reset_vector = read_internal_flash_int(FLASH_FIRMWARE_START + 4)

    # Get firmware size
    firmware_checksum_position = read_internal_flash_int(FLASH_FIRMWARE_START + 0x204)
    firmware_size = firmware_checksum_position + 4 - FLASH_FIRMWARE_START

    # Get firmware version
    firmware_version_position = read_internal_flash_int(FLASH_FIRMWARE_START + 0x200)
    firmware_version = read_internal_flash(firmware_version_position, 20).decode()

    return hub_id, firmware_version, firmware_size, firmware_reset_vector


def get_lego_firmware(size):
    """Reads the firmware that is currently running on the hub."""

    bytes_read = 0

    # Yield new blocks until done.
    while bytes_read < size:

        # Read several chunks of 32 bytes into one block.
        block = b""
        for i in range(FLASH_WRITE_SIZE // FLASH_READ_SIZE):
            block += firmware.flash_read(bytes_read)
            bytes_read += FLASH_READ_SIZE

        # If we read past the end, cut off the extraneous bytes.
        if bytes_read > size:
            block = block[0 : size % FLASH_WRITE_SIZE]

        # Yield the resulting block.
        yield block


def get_file_hash(path):
    """Gets file size and sha256 hash."""

    hash_calc = uhashlib.sha256()
    size = 0

    with open(path, "rb") as bin_file:
        data = b"START"
        while len(data) > 0:
            data = bin_file.read(128)
            size += len(data)
            hash_calc.update(data)

    return (size, ubinascii.hexlify(hash_calc.digest()).decode())


def install(auto_reboot=True):

    # Get information about the running original LEGO firmware.
    hub_id, lego_version, lego_size, lego_reset_vector = get_lego_firmware_info()

    # Exit if user is running (outdated) Pybricks dual-boot firmware.
    if lego_reset_vector >= 0x80C0000:
        stop_installation(ERROR_DUAL_BOOT)

    # Print LEGO firmware information.
    print("Detected LEGO firmware:")
    print("    Version:", lego_version)
    print("    Size:", lego_size / 1024, "KB")

    # Back up the LEGO firmware so we can restore it from Pybricks later.
    print("Creating backup at:", LEGO_FIRMWARE_PATH)
    with open(LEGO_FIRMWARE_PATH, "wb") as backup_file:
        for block in get_lego_firmware(lego_size):
            _ = backup_file.write(block)

    # Save meta data along with firmware for additional checking.
    print("Creating meta data backup at:", LEGO_METADATA_PATH)
    with open(LEGO_METADATA_PATH, "w") as lego_meta_file:
        lego_backup_size, lego_backup_hash = get_file_hash(LEGO_FIRMWARE_PATH)
        lego_info = {
            "device-id": HUB_IDS[hub_id],
            "firmware-sha256": lego_backup_hash,
            "firmware-size": lego_size,
            "firmware-version": lego_version,
        }
        lego_meta_file.write(ujson.dumps(lego_info))

    # Get information about the Pybricks firmware.
    pybricks_size, pybricks_hash = get_file_hash(PYBRICKS_FIRMWARE_PATH)

    # Validate meta data
    with open(PYBRICKS_METADATA_PATH, "r") as meta_file:
        pybricks_info = ujson.loads(meta_file.read())

        # Verify file integrity using sha256 from metadata.
        if pybricks_hash != pybricks_info["firmware-sha256"]:
            stop_installation(ERROR_BAD_FIRMWARE)

        # Check that this firmware is made for this hub.
        if HUB_IDS[hub_id] != pybricks_info["device-id"]:
            stop_installation(ERROR_BAD_HUB)

    # Display Pybricks firmware information.
    print("Detected Pybricks firmware:")
    print("    Version:", pybricks_info["firmware-version"])
    print("    Size:", pybricks_size / 1024, "KB")

    # Initialize the firmware boot partition.
    print("Preparing external storage.")
    if pybricks_size < 128 * 1024:
        stop_installation(ERROR_BAD_FIRMWARE)
    if not firmware.appl_image_initialise(pybricks_size):
        stop_installation(ERROR_EXTERNAL_FLASH)

    # Copy the Pybricks firmware to external flash.
    print("Installing Pybricks firmware.")
    with open(PYBRICKS_FIRMWARE_PATH, "rb") as pybricks_firmware_file:
        while True:
            block = pybricks_firmware_file.read(FLASH_WRITE_SIZE)
            if len(block) == 0:
                break
            _ = firmware.appl_image_store(block)

    # Check the firmware file on external flash.
    if not firmware.info()["valid"]:
        stop_installation(ERROR_FIRMWARE_COPY_FAILED)

    # Print success.
    print("Success! The firmware will be installed when it reboots.")

    # Reboot or not.
    if auto_reboot:
        # Reboot soon, giving some time to softly disconnect.
        print("Rebooting soon! Please wait.")
        timer = umachine.Timer()
        timer.init(period=1500, mode=umachine.Timer.ONE_SHOT, callback=lambda x: umachine.reset())
    else:
        # Don't reboot. This is useful for debugging.
        print("Not rebooting.")


# Run as main script for debugging purposes.
if __name__ == "__main__":

    install(auto_reboot=False)
