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
SLOT = 0

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

SLOT = {slot}
PYBRICKS_SIZE = {size}
EXTRA_SIZE = 4
BLOCK_WRITE = {block_write}
PYBRICKS_VECTOR = {pybricks_vector}
BLOCK_READ = 32
reads_per_write = BLOCK_WRITE // BLOCK_READ
FF = bytes([255])

# Read boot data from the currently running firmware.
PYBRICKS_BASE = 0x80C0000
OFFSET = 0x8008000
start_data = flash_read(0x000)
boot_data = flash_read(0x200)

# Store Pybricks starting on final section of 256K free space
pybricks_start = PYBRICKS_BASE - OFFSET
pybricks_end = pybricks_start + PYBRICKS_SIZE
total_end = pybricks_end + EXTRA_SIZE

firmware_version_address = int.from_bytes(boot_data[0:4], 'little') - OFFSET
firmware_version = flash_read(firmware_version_address)[0:20]
checksum_address = int.from_bytes(boot_data[4:8], 'little') - OFFSET
checksum_value = int.from_bytes(flash_read(checksum_address)[0:4], 'little')
firmware_end_address = checksum_address + 4

# Read boot vector
firmware_boot_vector = start_data[4:8]
if int.from_bytes(firmware_boot_vector, 'little') > PYBRICKS_BASE:
    # Boot vector was pointing at Pybricks, so we need to read the backup
    firmware_boot_vector = flash_read(pybricks_start - 4)[0:4]

if firmware_boot_vector == FF*4:
    raise ValueError("Could not find reset vector.")

# Original firmware starts directly after bootloader. This is where flash_read
# has index 0.
next_read_index = 0

print('Creating empty space for backup')

# Initialize external flash up to the end of Pybricks
appl_image_initialise(total_end)

print('Begin backup of original firmware.')

# Copy internal flash to external flash in order to back up original firmware.
progress_print = None
while next_read_index < checksum_address:
    # Read several chunks of 32 bytes into one block to write
    block = b''
    for i in range(reads_per_write):
        block += flash_read(next_read_index)
        next_read_index += BLOCK_READ

    # We modify the very first block to change the boot vector
    if next_read_index == BLOCK_WRITE:
        block = block[0:4] + PYBRICKS_VECTOR.to_bytes(4, 'little') + block[8:]

    # Write the whole block, except if at or beyond checksum
    if next_read_index < checksum_address:
        appl_image_store(block)
    else:
        block_end = BLOCK_WRITE - next_read_index + checksum_address
        appl_image_store(block[0:block_end])
        next_read_index = checksum_address

    # Display progress
    progress = (next_read_index*100)//checksum_address
    if progress != progress_print:
        print(progress)
        progress_print = progress

# If we had kept track of CRC32 until this point, now would be the time
# to compare it to the checksum

# Finally we can write the original checksum itself
appl_image_store(flash_read(checksum_address)[0:4])
next_read_index += 4

# Add padding to the next whole write block
if (firmware_end_address % BLOCK_WRITE) != 0:
    padding = BLOCK_WRITE - (firmware_end_address % BLOCK_WRITE)
    next_read_index += padding
    appl_image_store(FF*padding)
print(info())

print('Skipping empty space.')

# Add padding up until the start of Pybricks
ff_block = FF * BLOCK_WRITE

while next_read_index != pybricks_start - BLOCK_WRITE:
    appl_image_store(ff_block)
    next_read_index += BLOCK_WRITE

# In the last padding block, include a backup of the original reset vector
appl_image_store(FF * (BLOCK_WRITE - 4) + firmware_boot_vector)

print(info())

print('Opening Pybricks firmware.')

# Open the slots file to determine the name of the current script.
with open('projects/.slots', 'r') as slots_file:
    slot_info = eval(slots_file.read())
    current_script_name = 'projects/' + str(slot_info[SLOT]['id']) + '.py'

# Open the current script
script = open(current_script_name, 'rb')

# Set read index to start of binary
while script.readline().strip() != b'# ___FIRMWARE_BEGIN___':
    pass

print('Begin backup of Pybricks firmware.')

bytes_done = 0
progress_print = None

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
    appl_image_store(decoded)

    # Show progress
    bytes_done += len(decoded)
    progress = (bytes_done*100)//PYBRICKS_SIZE
    if progress != progress_print:
        print(progress)
        progress_print = progress

overall_checksum = info()['new_appl_image_calc_checksum']
appl_image_store(overall_checksum.to_bytes(4, 'little'))
result = info()

if result['valid'] == 1:
    print('Succes! The firmware will be installed after reboot.')
    reset()
else:
    print('Could not back up the firmware. No changed will be made.')
    print(result)

""".format(
    slot=SLOT,
    size=len(pybricks_bin),
    version=version,
    block_write=BLOCK_WRITE_SIZE,
    pybricks_vector=int.from_bytes(pybricks_bin[4:8], "little"),
)

# Write script to a Python file
with open(path.join(BUILD_PATH, "llsp_install_pybricks.py"), "w") as installer:

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
with open(path.join(BUILD_PATH, "llsp_manifest.json"), "w") as manifest:
    manifest.write(dumps(MANIFEST))

# Convert the Python script to JSON format. New line symbols get messy, so
# that's why we avoid them in the user script above.
with open(path.join(BUILD_PATH, "llsp_install_pybricks.py"), "r") as installer:
    with open(path.join(BUILD_PATH, "llsp_projectbody.json"), "w") as body:
        blob = installer.read().replace("\n", "\\n")
        body.write('{{"program":"{0}"}}'.format(blob))

# Combine all files in an LLSP archive
llsp = ZipFile(path.join(BUILD_PATH, "install_pybricks.llsp"), "w")
llsp.write(path.join(BUILD_PATH, "llsp_projectbody.json"), "projectbody.json")
llsp.write(path.join(BUILD_PATH, "llsp_manifest.json"), "manifest.json")
llsp.write(path.join(TOOLS_PATH, "pybricks.svg"), "icon.svg")
llsp.close()
