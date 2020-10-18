#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

"""Builds an end-user project file to install Pybricks on SPIKE Prime."""

from zipfile import ZipFile
from base64 import b64encode
from os import path
from json import dumps
from sys import argv

BUILD_PATH = "build"
TOOLS_PATH = "../../tools"

# Get release and git version
version = argv[1]

# User program slot
SLOT = 19

# Manifest file that goes into the project zip
MANIFEST = {
    "type": "python",
    "autoDelete": False,
    "created": "2020-09-28T16:55:24.744Z",
    "id": "PtOPQLLB94ax",
    "lastsaved": "2020-09-28T19:48:11.098Z",
    "size": 0,
    "name": "test",
    "slotIndex": SLOT,
    "zoomLevel": 0.5,
    "hardware": {"python": {}},
    "state": {},
}

# How many bytes to write to external flash in one go (multiple of 32).
BLOCK_WRITE_SIZE = 128

# Read the Pybricks firmware.
with open(path.join(BUILD_PATH, "firmware-dual-boot-base.bin"), "rb") as firmware:
    pybricks_bin = firmware.read()

# This is the main script that will be run inside the SPIKE Prime app.
INSTALL_SCRIPT = """\
# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 The Pybricks Authors

# Pybricks installer for SPIKE Prime
# Version: {version}

from firmware import appl_image_initialise, appl_image_store, info, flash_read
from ubinascii import a2b_base64
from umachine import reset
from util import storage
from spike import LightMatrix
from utime import sleep_ms

SLOT = {slot}
PYBRICKS_SIZE = {size}
BLOCK_WRITE = {block_write}
PYBRICKS_VECTOR = {pybricks_vector}

BLOCK_READ = 32
PYBRICKS_BASE = 0x80C0000
FLASH_OFFSET = 0x8008000
reads_per_write = BLOCK_WRITE // BLOCK_READ

FF = bytes([255])

# Function to display progress
lights = LightMatrix()
progress_shown = [0, 0, 0, 0, 0]
def show_progress(row, progress):
    pixels = int(progress)//20
    if pixels > progress_shown[row]:
        progress_shown[row] = pixels
        for col in range(pixels):
            lights.set_pixel(col, row)


def get_base_firmware_boot_vector():
    # Read from base firmware location
    firmware_boot_vector = flash_read(0x000)[4:8]

    # If it's not pointing at Pybricks, return as is.
    if int.from_bytes(firmware_boot_vector, 'little') < PYBRICKS_BASE:
        return firmware_boot_vector

    # Otherwise read the boot vector in Pybricks
    pybricks_boot_vector = flash_read(PYBRICKS_BASE - FLASH_OFFSET - 4)[0:4]

    # We also read it from the back up location as a safety check
    backup_boot_vector = flash_read(PYBRICKS_BASE - FLASH_OFFSET + 4)[0:4]

    # They must be equal and not empty
    if pybricks_boot_vector != backup_boot_vector or backup_boot_vector == 4 * FF:
        raise ValueError('Could not read boot vector.')

    # Return result
    return pybricks_boot_vector


def initialize_flash():

    print('Creating empty space for backup')

    # Show initial progress to indicate we are on our way.
    for i in range(4):
        show_progress(0, i*20+20)
        sleep_ms(500)

    # Initialize external flash up to the end of Pybricks
    # with four extra bytes for the overall checksum
    # This is blocking, so we cannot update progress.
    appl_image_initialise(PYBRICKS_BASE + PYBRICKS_SIZE - FLASH_OFFSET + 4)
    show_progress(0, 100)


# Gets the original firmware with the updated boot vector.
def get_base_firmware():

    # Read current base firmware version
    boot_data = flash_read(0x200)
    firmware_version_position = int.from_bytes(boot_data[0:4], 'little') - FLASH_OFFSET
    firmware_version = flash_read(firmware_version_position)[0:20]
    print('Begin reading original firmware:', firmware_version)

    # Read where the checksum is so we know the base firmware size
    checksum_position = int.from_bytes(boot_data[4:8], 'little') - FLASH_OFFSET
    base_firmware_size = checksum_position + 4

    bytes_read = 0

    # Yield new blocks until done
    while bytes_read < base_firmware_size:

        # Read several chunks of 32 bytes into one block.
        block = b''
        for i in range(reads_per_write):
            block += flash_read(bytes_read)
            bytes_read += BLOCK_READ

        # The first block is updated with the desired boot vector.
        if bytes_read == BLOCK_WRITE:
            block = block[0:4] + PYBRICKS_VECTOR.to_bytes(4, 'little') + block[8:]

        # If we read past the end, cut off the extraneous bytes
        if bytes_read > base_firmware_size:
            block = block[0 : base_firmware_size % BLOCK_WRITE]

        # Yield the resulting block
        yield block


# Gets empty padding blocks with extra information put in at the end
def get_padding(padding_length, extra_info):

    if len(extra_info) > padding_length:
        raise ValueError('Padding not large enough for extra data.')

    # Total padding size
    padding_ff = padding_length - len(extra_info)

    # Pad whole blocks as far as we can
    for _ in range(padding_ff // BLOCK_WRITE):
        yield FF * BLOCK_WRITE

    # Pad remaining FF as a partial block
    yield FF * (padding_ff % BLOCK_WRITE)

    # Padd the extra info
    yield extra_info

def get_combined_firmware():

    base_firmware_size = 0
    base_firmware_boot_vector = get_base_firmware_boot_vector()

    for block in get_base_firmware():
        base_firmware_size += len(block)
        yield block

    for block in get_padding(PYBRICKS_BASE - FLASH_OFFSET - base_firmware_size, base_firmware_boot_vector):
        yield block

# Get external flash ready
initialize_flash()

# Write the combined firmware to external flash
bytes_written = 0

# Read base firmware and copy it to external flash, including padding to next
for block in get_combined_firmware():

    # Store the block from internal flash on external flash
    if len(block) > 0:
        appl_image_store(block)

    # Display progress
    bytes_written += len(block)
    # show_progress(1, FIXME)

print('Opening Pybricks firmware.')
# Open the current script
script = open(storage.get_path(SLOT) + '.py', 'rb')

# Set read index to start of binary
while script.readline().strip() != b'# ___FIRMWARE_BEGIN___':
    pass

print('Begin backup of Pybricks firmware.')

bytes_done = 0

# Save binary to external flash
while True:
    # Read next line
    line = script.readline()

    # Stop if we are at the end
    if line.strip() == b'# ___FIRMWARE_END___':
        break

    # Get the base64 string
    base64 = line[2:len(line)-1]

    # Decode and write
    decoded = a2b_base64(base64)

    # In the first block, override the boot vector
    if bytes_done == 0:
        decoded = decoded[0:4] + get_base_firmware_boot_vector() + decoded[8:]

    appl_image_store(decoded)

    # Show progress
    bytes_done += len(decoded)
    show_progress(2, (bytes_done*100)//PYBRICKS_SIZE)

overall_checksum = info()['new_appl_image_calc_checksum']
appl_image_store(overall_checksum.to_bytes(4, 'little'))
result = info()

if result['valid'] == 1:
    print('Succes! The firmware will be installed after reboot.')
    reset()
else:
    print('Could not back up the firmware. No changes will be made.')
    print(result)

""".format(
    slot=SLOT,
    size=len(pybricks_bin),
    version=version,
    block_write=BLOCK_WRITE_SIZE,
    pybricks_vector=int.from_bytes(pybricks_bin[4:8], "little"),
)

# Write script to a Python file
with open(path.join(BUILD_PATH, "dual_boot_install_pybricks.py"), "w") as installer:

    # Write the main code
    installer.write(INSTALL_SCRIPT)

    # Write flag to indicate firmware start
    installer.write("# ___FIRMWARE_BEGIN___\n")

    # Write binary segment in base64 format as a comment
    done = 0
    while done != len(pybricks_bin):
        block = pybricks_bin[done : done + BLOCK_WRITE_SIZE]
        encoded = b64encode(block)
        installer.write("# {0}\n".format(encoded.decode("ascii")))
        done += len(block)

    # Write flag to indicate firmware end
    installer.write("# ___FIRMWARE_END___\n")

# Write the manifest to file
with open(path.join(BUILD_PATH, "dual_boot_manifest.json"), "w") as manifest:
    manifest.write(dumps(MANIFEST))

# Convert the Python script to JSON format. New line symbols get messy, so
# that's why we avoid them in the user script above.
with open(path.join(BUILD_PATH, "dual_boot_install_pybricks.py"), "r") as installer:
    with open(path.join(BUILD_PATH, "dual_boot_projectbody.json"), "w") as body:
        blob = installer.read().replace("\n", "\\n")
        body.write('{{"program":"{0}"}}'.format(blob))

# Combine all files in the project archive
for ext in ("llsp", "lms"):
    archive = ZipFile(path.join(BUILD_PATH, "install_pybricks." + ext), "w")
    archive.write(path.join(BUILD_PATH, "dual_boot_projectbody.json"), "projectbody.json")
    archive.write(path.join(BUILD_PATH, "dual_boot_manifest.json"), "manifest.json")
    archive.write(path.join(TOOLS_PATH, "pybricks.svg"), "icon.svg")
    archive.close()
