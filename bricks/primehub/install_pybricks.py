# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2021 The Pybricks Authors
#
# Pybricks dual boot installer for SPIKE Prime and MINDSTORMS Robot Inventor.


import firmware
import hub

import ubinascii
import umachine
import utime
import uhashlib
import uos
import micropython


FLASH_START = 0x8000000
FLASH_END = FLASH_START + 1024 * 1024

FLASH_LEGO_START = 0x8008000
FLASH_PYBRICKS_START = 0x80C0000

BLOCK_READ_SIZE = 32
BLOCK_WRITE_SIZE = BLOCK_READ_SIZE * 4

FF = b"\xFF"


KNOWN_LEGO_FIRMWARE_VERSIONS = {
    b"v1.2.01.0103-d08b6fe": "MINDSTORMS Robot Inventor App v1.3.4 / 10.1.0",
}


def read_flash(address, length):
    """Read a given number of bytes from a given absolute address."""
    return firmware.flash_read(address - FLASH_LEGO_START)[0:length]


def read_flash_int(address):
    """Gets a little endian uint32 integer from the internal flash."""
    return int.from_bytes(read_flash(address, 4), "little")


def get_pybricks_reset_vector():
    """Gets the boot vector of the pybricks firmware."""

    # Extract reset vector from dual boot firmware.
    with open("_pybricks/firmware-dual-boot-base.bin", "rb") as pybricks_bin_file:
        pybricks_bin_file.seek(4)
        return pybricks_bin_file.read(4)


def get_lego_reset_vector():
    """Gets the boot vector of the original firmware."""

    # Read from lego firmware location.
    reset_vector = read_flash(FLASH_LEGO_START + 4, 4)

    # If it's not pointing at Pybricks, return as is.
    if int.from_bytes(reset_vector, "little") < FLASH_PYBRICKS_START:
        return reset_vector

    # Otherwise read the reset vector in the dual-booted Pybricks that is
    # already installed, which points to the LEGO firmware.
    return read_flash(FLASH_PYBRICKS_START + 4, 4)


def get_lego_firmware(size, reset_vector):
    """Gets the LEGO firmware with an updated reset vector."""

    bytes_read = 0

    # Yield new blocks until done.
    while bytes_read < size:

        # Read several chunks of 32 bytes into one block.
        block = b""
        for i in range(BLOCK_WRITE_SIZE // BLOCK_READ_SIZE):
            block += firmware.flash_read(bytes_read)
            bytes_read += BLOCK_READ_SIZE

        # The first block is updated with the desired boot vector.
        if bytes_read == BLOCK_WRITE_SIZE:
            block = block[0:4] + reset_vector + block[8:]

        # If we read past the end, cut off the extraneous bytes.
        if bytes_read > size:
            block = block[0 : size % BLOCK_WRITE_SIZE]

        # Yield the resulting block.
        yield block


def get_pybricks_firmware(reset_vector):
    """Gets the Pybricks firmware with an updated reset vector."""

    # Open the file and get it chunk by chunk
    with open("_pybricks/firmware-dual-boot-base.bin", "rb") as pybricks_bin_file:

        # Read first chunk and override boot vector
        block = bytearray(pybricks_bin_file.read(BLOCK_WRITE_SIZE))
        block[4:8] = reset_vector
        yield bytes(block)

        # Yield remaining blocks
        while len(block) > 0:
            block = pybricks_bin_file.read(BLOCK_WRITE_SIZE)
            yield block


def get_padding(padding_length):
    """Gets empty padding blocks with extra information put in at the end."""

    # Pad whole blocks as far as we can.
    for _ in range(padding_length // BLOCK_WRITE_SIZE):
        yield FF * BLOCK_WRITE_SIZE

    # Pad remaining FF as a partial block.
    yield FF * (padding_length % BLOCK_WRITE_SIZE)


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


def install(check_lego_firmware_version=True):
    """Main installation routine."""

    print("Starting installation script.")

    # Get hash of uploaded files.
    print("Checking installation files.")
    pybricks_size, pybricks_hash_calc = get_file_hash("_pybricks/firmware-dual-boot-base.bin")
    script_size, script_hash_calc = get_file_hash("_pybricks/install_pybricks.py")

    # Read what the hashes should be.
    with open("_pybricks/install_pybricks_hash.txt") as hash_file:
        pybricks_hash_read = hash_file.readline().strip()
        script_hash_read = hash_file.readline().strip()

    # Check if hashes match.
    if pybricks_hash_read == pybricks_hash_calc and script_hash_read == script_hash_calc:
        print("Files looking good!")
    else:
        print("The installation files are corrupt.")
        return

    # Get firmware information.
    print("Getting firmware info.")
    lego_checksum_position = read_flash_int(FLASH_LEGO_START + 0x204)
    lego_size = lego_checksum_position + 4 - FLASH_LEGO_START

    lego_version_position = read_flash_int(FLASH_LEGO_START + 0x200)
    lego_version = read_flash(lego_version_position, 20)
    print("Your hub runs LEGO Firmware version:", lego_version)

    if check_lego_firmware_version and lego_version not in KNOWN_LEGO_FIRMWARE_VERSIONS:
        print("Your LEGO firmware version is not (yet) supported.")
        print("Please install one of the following versions and try again.\n")

        for version, source in KNOWN_LEGO_FIRMWARE_VERSIONS.items():
            print("    {0} from {1}.".format(version.decode(), source))

        print(
            "\nIf you have a newer version that is not listed, please ask for support in https://github.com/pybricks/support/issues/167"
        )
        return

    # Verify firmware sizes
    if FLASH_LEGO_START + lego_size >= FLASH_PYBRICKS_START:
        print("LEGO firmware too big.")
        return

    # Total size of combined firmwares, including checksum
    total_size = FLASH_PYBRICKS_START - FLASH_LEGO_START + pybricks_size + 4
    if total_size >= FLASH_END:
        print("Pybricks firmware too big.")
        return

    # Initialize flash
    print("Initializing flash for {0} bytes.".format(total_size))
    if total_size < 512 or not firmware.appl_image_initialise(total_size):
        print("Failed to initialize external flash.")
        return

    # Copy original firmware to external flash
    print("Copying LEGO firmware.")
    for block in get_lego_firmware(lego_size, get_pybricks_reset_vector()):
        firmware.appl_image_store(block)

    # Add padding to external flash
    print("Copying padding.")
    for block in get_padding(FLASH_PYBRICKS_START - FLASH_LEGO_START - lego_size):
        firmware.appl_image_store(block)

    # Copy pybricks firmware to external flash
    print("Copying Pybricks firmware.")
    for block in get_pybricks_firmware(get_lego_reset_vector()):
        firmware.appl_image_store(block)

    # Get the combined checksum and store it.
    overall_checksum = firmware.info()["new_appl_image_calc_checksum"]
    firmware.appl_image_store(overall_checksum.to_bytes(4, "little"))

    # Check result
    if firmware.info()["valid"]:
        print("Success! The firmware will be installed when it reboots.")
    else:
        print("Firmware image not accepted. It will not be installed.")

    # Skip reboot if right button is held.
    if hub.button.right.is_pressed():
        print("Skipping reboot.")
        return

    # Reboot soon, giving some time to softly disconnect.
    print("Rebooting soon! Please wait.")
    timer = umachine.Timer()
    timer.init(period=1500, mode=umachine.Timer.ONE_SHOT, callback=lambda x: umachine.reset())
