#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

"""Builds an end-user project file to install Pybricks on SPIKE Prime."""

from zipfile import ZipFile
from base64 import b64encode
from os import path
from json import dumps

BUILD_PATH = "build"
TOOLS_PATH = "../../tools"

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

# How many bytes to write to external flash in one go.
BLOCK_SIZE = 128

# Read the Pybricks firmware.
with open(path.join(BUILD_PATH, "firmware.bin"), "rb") as firmware:
    pybricks_bin = firmware.read()

# This is the main script that will be run inside the SPIKE Prime app.
INSTALL_SCRIPT = """# Pybricks installer for SPIKE Prime

from firmware import appl_image_initialise, appl_image_store, info
from ubinascii import a2b_base64
from umachine import reset

SLOT = {slot}
SIZE = {size}

# Open the slots file to determine the name of the current script.
with open('projects/.slots', 'r') as slots_file:
    slot_info = eval(slots_file.read())
    current_script_name = 'projects/' + str(slot_info[SLOT]['id']) + '.py'

# Open the current script
script = open(current_script_name, 'rb')

# Set read index to start of binary
while script.readline().strip() != b'# ___FIRMWARE_BEGIN___':
    pass

print('Prepare copy')

# Initialize external flash
appl_image_initialise(SIZE)

print('Begin copy')

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
    progress = (bytes_done*100)//SIZE
    if progress != progress_print:
        print(progress)
        progress_print = progress

print('DONE. REBOOTING. DO NOT REMOVE BATTERIES.')
print(info())
reset()

""".format(
    slot=SLOT, size=len(pybricks_bin)
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
        block = pybricks_bin[done : done + BLOCK_SIZE]
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
