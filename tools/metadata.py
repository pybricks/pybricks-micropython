#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2023 The Pybricks Authors

"""
Pybricks firmware metadata file generation tool.

Generates a .json file with information about a Pybricks firmware binary blob.

v2.1.0:
    metadata-version    string  "2.1.0"
    firmware-version    string  output of `git describe --tags --dirty`
    device-id           number  one of [0x40, 0x41, 0x80, 0x81, 0x83, 0xE0, 0xE1, 0xE2]
    checksum-type       string  one of ["sum", "crc32", "none"]
    checksum-size       number  size of flash memory for computing checksum
    hub-name-offset     number  offset in firmware.bin where hub name is located
    hub-name-size       number  number of bytes allocated for hub name

Device IDs:
    0x40: BOOST Move hub
    0x41: City hub
    0x80: Technic hub
    0x81: SPIKE Prime/MINDSTORMS Robot Inventor hub
    0x83: SPIKE Essential hub
    0xE0: MINDSTORMS RCX
    0xE1: MINDSTORMS NXT
    0xE2: MINDSTORMS EV3
"""

import argparse
import io
import json
import os
import re
import sys

# Path to repo top-level dir.
TOP = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "micropython"))
sys.path.append(os.path.join(TOP, "py"))
sys.path.append(os.path.join(TOP, "tools"))


# metadata file format version
VERSION = "2.1.0"

# hub-specific info
HUB_INFO = {
    "move_hub": {"device-id": 0x40, "checksum-type": "sum"},
    "city_hub": {"device-id": 0x41, "checksum-type": "sum"},
    "technic_hub": {"device-id": 0x80, "checksum-type": "sum"},
    "prime_hub": {"device-id": 0x81, "checksum-type": "crc32"},
    "essential_hub": {"device-id": 0x83, "checksum-type": "crc32"},
    "rcx": {"device-id": 0xE0, "checksum-type": "none"},
    "nxt": {"device-id": 0xE1, "checksum-type": "none"},
    "ev3rt": {"device-id": 0xE2, "checksum-type": "none"},
}


def generate(
    fw_version: str,
    hub_type: str,
    map_file: io.FileIO,
    out_file: io.FileIO,
):
    metadata = {
        "metadata-version": VERSION,
        "firmware-version": fw_version,
    }

    if hub_type not in HUB_INFO:
        print("Unknown hub type", file=sys.stderr)
        exit(1)

    metadata.update(HUB_INFO[hub_type])

    if (metadata["device-id"] & 0xE0) == 0xE0:
        # legacy hubs don't support these features
        metadata["checksum-size"] = 0
        metadata["hub-name-offset"] = 0
        metadata["hub-name-size"] = 0
    else:
        # scrape info from map file

        flash_origin = None  # Starting address of firmware area in flash memory
        flash_firmware_size = None  # Size of flash memory allocated to firmware
        flash_user_0_size = 0  # Size of flash memory within checksum area allocated to user
        name_start = None  # Starting address of custom hub name
        name_size = None  # Size reserved for custom hub name
        user_start = None  # Starting address of user .mpy file

        for line in map_file.readlines():
            match = re.match(
                r"^FLASH_FIRMWARE\s+(0x[0-9A-Fa-f]{8,16})\s+(0x[0-9A-Fa-f]{8,16})", line
            )
            if match:
                flash_origin = int(match[1], base=0)
                flash_firmware_size = int(match[2], base=0)
                continue

            match = re.match(
                r"^FLASH_USER_0\s+(0x[0-9A-Fa-f]{8,16})\s+(0x[0-9A-Fa-f]{8,16})", line
            )
            if match:
                flash_user_0_size = int(match[2], base=0)
                continue

            match = re.match(r"^\.name\s+(0x[0-9A-Fa-f]{8,16})\s+(0x[0-9A-Fa-f]+)", line)
            if match:
                name_start = int(match[1], base=0)
                name_size = int(match[2], base=0)
                continue

            match = re.match(r"^\.user\s+(0x[0-9A-Fa-f]{8,16})", line)
            if match:
                user_start = int(match[1], base=0)
                continue

        if flash_origin is None:
            print("Failed to find 'FLASH_FIRMWARE' start address", file=sys.stderr)
            exit(1)

        if flash_firmware_size is None:
            print("Failed to find 'FLASH_FIRMWARE' length", file=sys.stderr)
            exit(1)

        if name_start is None:
            print("Failed to find '.name' start address", file=sys.stderr)
            exit(1)

        if user_start is None:
            print("Failed to find '.user' start address", file=sys.stderr)
            exit(1)

        metadata["checksum-size"] = flash_firmware_size + flash_user_0_size
        metadata["hub-name-offset"] = name_start - flash_origin
        metadata["hub-name-size"] = name_size

    json.dump(metadata, out_file, indent=4, sort_keys=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate firmware metadata.")
    parser.add_argument(
        "fw_version",
        metavar="<firmware-version>",
        type=str,
        help="Pybricks firmware version",
    )
    parser.add_argument(
        "hub_type",
        metavar="<hub-type>",
        choices=HUB_INFO.keys(),
        help="hub type/device ID",
    )
    parser.add_argument(
        "map_file",
        metavar="<map-file>",
        type=argparse.FileType("r"),
        help="firmware linker map file name",
    )
    parser.add_argument(
        "out_file",
        metavar="<output-file>",
        type=argparse.FileType("w"),
        help="output file name",
    )

    args = parser.parse_args()
    generate(
        args.fw_version,
        args.hub_type,
        args.map_file,
        args.out_file,
    )
