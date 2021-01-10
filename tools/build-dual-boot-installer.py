#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

"""Builds an end-user project file to install Pybricks on SPIKE Prime."""

from zipfile import ZipFile
from base64 import b64encode
from os import path
from json import dumps
from sys import argv
from datetime import datetime

BUILD_PATH = "build"
TOOLS_PATH = "../../tools"

# Get release and git version.
version = argv[1][0:22]

# User program slot.
DOWNLOAD_SLOT = 18
INSTALL_SLOT = 19

# How many bytes to write to external flash in one go (multiple of 32).
BLOCK_WRITE_SIZE = 128
BASE64_BLOCK_WRITE_SIZE = len(b64encode(b"a" * BLOCK_WRITE_SIZE))

# Read the Pybricks firmware.
with open(path.join(BUILD_PATH, "firmware-dual-boot-base.bin"), "rb") as fw:
    pybricks_bin = fw.read()

# This is the main script that will be run inside the SPIKE Prime app.
INSTALL_SCRIPT = """\
# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 The Pybricks Authors
#
# Pybricks installer for SPIKE Prime and MINDSTORMS Robot Inventor.
# Version: {version}

from firmware import appl_image_initialise, appl_image_store, info, flash_read
from ubinascii import a2b_base64
from umachine import reset
from util import storage
from spike import LightMatrix
from utime import sleep_ms

# Firmware information.
SLOT = {slot}
PYBRICKS_SIZE = {size}
BLOCK_WRITE = {block_write}
BASE64_BLOCK_WRITE = {base64_block_write}
PYBRICKS_VECTOR = {pybricks_vector}
VERSION = b'{version}'

# Settings for reading and writing to external storage.
BLOCK_READ = 32
PYBRICKS_BASE = 0x80C0000
FLASH_OFFSET = 0x8008000
reads_per_write = BLOCK_WRITE // BLOCK_READ
total_fw_size = PYBRICKS_BASE + PYBRICKS_SIZE - FLASH_OFFSET + 4
FF = bytes([255])

# Initialize light matrix.
lights = LightMatrix()
pixels_on = 0


def show_progress(progress):
    '''Shows installation progress on the light matrix.'''
    global pixels_on
    pixels_now = int(progress) // 4
    if pixels_now > pixels_on:
        for i in range(pixels_on, pixels_now):
            lights.set_pixel(i % 5, i // 5)


def get_base_firmware_boot_vector():
    '''Gets the boot vector of the original firmware.'''

    # Read from base firmware location.
    firmware_boot_vector = flash_read(0x000)[4:8]

    # If it's not pointing at Pybricks, return as is.
    if int.from_bytes(firmware_boot_vector, 'little') < PYBRICKS_BASE:
        return firmware_boot_vector

    # Otherwise read the boot vector in Pybricks.
    pybricks_boot_vector = flash_read(PYBRICKS_BASE - FLASH_OFFSET - 4)[0:4]

    # We also read it from the back up location as a safety check.
    alt_boot_vector = flash_read(PYBRICKS_BASE - FLASH_OFFSET + 4)[0:4]

    # They must be equal and not empty.
    if pybricks_boot_vector != alt_boot_vector or alt_boot_vector == 4 * FF:
        raise ValueError('Could not read boot vector.')

    # Return result.
    return pybricks_boot_vector


def initialize_flash():
    '''Creates empty space on external flash to store the combined firmware.'''

    print('Creating empty space for backup')

    # Show initial progress to indicate we are on our way.
    for i in range(4):
        show_progress(i*4+4)
        sleep_ms(1000)

    # Initialize external flash up to the end of Pybricks.
    # This is blocking, so we cannot update progress.
    appl_image_initialise(total_fw_size)
    show_progress(20)


def get_base_firmware():
    '''Gets the original firmware with the updated Pybricks boot vector.'''

    # Read current base firmware version.
    boot_data = flash_read(0x200)
    version_position = int.from_bytes(boot_data[0:4], 'little') - FLASH_OFFSET
    version = flash_read(version_position)[0:20]
    print('Begin reading original firmware:', version)

    # Read where the checksum is so we know the base firmware size.
    checksum_position = int.from_bytes(boot_data[4:8], 'little') - FLASH_OFFSET
    base_firmware_size = checksum_position + 4

    bytes_read = 0

    # Yield new blocks until done.
    while bytes_read < base_firmware_size:

        # Read several chunks of 32 bytes into one block.
        block = b''
        for i in range(reads_per_write):
            block += flash_read(bytes_read)
            bytes_read += BLOCK_READ

        # The first block is updated with the desired boot vector.
        if bytes_read == BLOCK_WRITE:
            block = block[0:4] + PYBRICKS_VECTOR.to_bytes(4, 'little') + block[8:]

        # If we read past the end, cut off the extraneous bytes.
        if bytes_read > base_firmware_size:
            block = block[0 : base_firmware_size % BLOCK_WRITE]

        # Yield the resulting block.
        yield block


def get_padding(padding_length, extra_info):
    '''Gets empty padding blocks with extra information put in at the end.'''

    if len(extra_info) > padding_length:
        raise ValueError('Padding not large enough for extra data.')

    # Total padding size.
    padding_ff = padding_length - len(extra_info)

    # Pad whole blocks as far as we can.
    for _ in range(padding_ff // BLOCK_WRITE):
        yield FF * BLOCK_WRITE

    # Pad remaining FF as a partial block.
    yield FF * (padding_ff % BLOCK_WRITE)

    # Padd the extra info.
    yield extra_info


def get_pybricks_firmware_lines():
    print('Opening Pybricks firmware.')
    download_project_path = storage.get_path(SLOT)
    if download_project_path is None:
        raise OSError('You must download the firmware before you can install it.')

    try:
        # Search for mpy file
        mpy_file = open(download_project_path + '/__init__.mpy', 'rb')
        print('Opened .mpy project')

        # Verify that this is a compatible mpy file
        mpy_version = mpy_file.read(2)
        if mpy_version != bytes((ord('M'), 5)):
            raise OSError('Incompatible .mpy format')

        TRIGGER = b'START_PYBRICKS_FIRMWARE_BINARY'
        triger_size = len(TRIGGER)

        # Search for start of binary
        for i in range(30):
            data = mpy_file.read(1000)
            position = mpy_file.tell()

            if TRIGGER not in data:
                mpy_file.seek(position - triger_size)
            else:
                idx = data.index(TRIGGER)
                mpy_file.seek(position - len(data) + idx + triger_size)
                download_version = mpy_file.read(len(VERSION))
                break

        if download_version != VERSION:
            raise OSError('Download and installation script do not match')

        print('Firmware version:', VERSION)

        # Read all the raw strings
        while True:
            # Skip padding
            if not mpy_file.read(3):
                break
            # Yield the string
            yield mpy_file.read(BASE64_BLOCK_WRITE)

    except OSError:
        # Search for plain Python file instead
        try:
            script = open(download_project_path + '.py', 'rb')
            print('Opened plain .py file')
        except OSError:
            script = open(download_project_path + '/__init__.py', 'rb')
            print('Opened plain .py project')

        # Set read index to start of binary.
        for i in range(10):
            line = script.readline().strip()
            if b'# Version:' in line:
                download_version = line[11:]
                script.readline()
                break

        if download_version != VERSION:
            raise OSError('Download and installation script do not match')

        print('Firmware version:', VERSION)

        # Trim and yield all the lines
        while line:
            line = script.readline()
            yield line[6:len(line)-2]


def get_pybricks_firmware(base_firmware_boot_vector):
    '''Reads the Pybricks firmware from the data strings in this file.'''

    # Open script reader
    lines = get_pybricks_firmware_lines()

    # Read first line and decode it.
    decoded = a2b_base64(next(lines))
    bytes_done = len(decoded)

    # Return the first block with the updated boot vector.
    yield decoded[0:4] + base_firmware_boot_vector + decoded[8:]

    # Read Pybricks binary.
    while bytes_done < PYBRICKS_SIZE:

        # Read next line and decode it.
        decoded = a2b_base64(next(lines))

        # Track progress.
        bytes_done += len(decoded)

        # Yield result.
        yield decoded


def get_combined_firmware():
    '''Glues the base firmware, padding, and Pybricks firmware to one blob.'''

    base_firmware_size = 0
    base_firmware_boot_vector = get_base_firmware_boot_vector()

    for block in get_base_firmware():
        base_firmware_size += len(block)
        yield block

    padding_length = PYBRICKS_BASE - FLASH_OFFSET - base_firmware_size
    for block in get_padding(padding_length, base_firmware_boot_vector):
        yield block

    for block in get_pybricks_firmware(base_firmware_boot_vector):
        yield block


# The main script starts here. First, get external flash ready.
initialize_flash()
bytes_written = 0

# Read base firmware and copy it to external flash, including padding to next.
for block in get_combined_firmware():

    # Store the block from internal flash on external flash.
    if len(block) > 0:
        appl_image_store(block)

    # Display progress.
    bytes_written += len(block)
    show_progress(bytes_written * 100 // total_fw_size)

# Get the combined checksum and store it.
overall_checksum = info()['new_appl_image_calc_checksum']
appl_image_store(overall_checksum.to_bytes(4, 'little'))

# Verify that all is well and done.
result = info()

if result['valid'] == 1:
    # Show final progress and then reboot.
    print('Succes! The firmware will be installed after reboot.')
    show_progress(100)
    sleep_ms(1000)
    reset()
else:
    # Failure, so remove the new copy. This prevents any updates.
    appl_image_initialise(0)

    # Indicate failure and output the firmware status.
    print('Could not back up the firmware. No changes will be made.')
    print(result)

""".format(
    slot=DOWNLOAD_SLOT,
    size=len(pybricks_bin),
    version=version,
    block_write=BLOCK_WRITE_SIZE,
    base64_block_write=BASE64_BLOCK_WRITE_SIZE,
    pybricks_vector=int.from_bytes(pybricks_bin[4:8], "little"),
)


def make_project_files(build_dir, project_name, script, slot):
    """Converts a Python script to .llsp and .lms archive formats."""

    # Get time in compatible format
    time_now = datetime.now().strftime("%Y-%m-%dT%H:%M:%S.000Z")

    # Manifest file that goes into the project zip.
    manifest = {
        "type": "python",
        "autoDelete": False,
        "created": time_now,
        "id": "PtOPQLLB94ax",
        "lastsaved": time_now,
        "size": 0,
        "name": project_name,
        "slotIndex": slot,
        "zoomLevel": 0.5,
        "hardware": {"python": {}},
        "state": {},
    }

    # Convert the Python script to the expected JSON format.
    blob = script.replace("\n", "\\n")

    # Combine all files in the project archive.
    for ext in (".llsp", ".lms"):

        if ext == ".llsp":
            projectbody = '{{"main":"{0}"}}'.format(blob)
        else:
            projectbody = '{{"program":"{0}"}}'.format(blob)

        archive = ZipFile(path.join(build_dir, project_name + ext), "w")
        archive.writestr("projectbody.json", projectbody)
        archive.writestr("manifest.json", dumps(manifest))
        archive.write(path.join(TOOLS_PATH, "pybricks.svg"), "icon.svg")
        archive.close()


make_project_files(BUILD_PATH, "install_pybricks", INSTALL_SCRIPT, INSTALL_SLOT)

# This script contains the Pybricks binary
DOWNLOAD_SCRIPT_HEADER = """\
# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 The Pybricks Authors
#
# Pybricks firmware for SPIKE Prime and MINDSTORMS Robot Inventor.
# Version: {version}
_d = 'START_PYBRICKS_FIRMWARE_BINARY{version}'
""".format(
    version=version
)

# Script starts with header, then binary data
download_script = DOWNLOAD_SCRIPT_HEADER

# Write binary segment in base64 format as a comment.
done = 0
while done != len(pybricks_bin):
    block = pybricks_bin[done : done + BLOCK_WRITE_SIZE]
    encoded = b64encode(block)
    encoded += b"=" * (BASE64_BLOCK_WRITE_SIZE - len(encoded))
    download_script += "_d = '{0}'\n".format(encoded.decode("ascii"))
    done += len(block)

make_project_files(BUILD_PATH, "download_pybricks", download_script, DOWNLOAD_SLOT)
