#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

"""
Pybricks firmware metadata file generation tool.

Generates a .json file with information about a Pybricks firmware binary blob.

v1.0.0:

    metadata-version    "1.0.0"
    firmware-version    output of `git describe --tags --dirty`
    device-id           one of 0x40, 0x41, 0x80, 0x00
    checksum-type       one of "xor", "crc32"
    user-mpy-offset     number
    max-firmware-size   number
"""

import argparse
import io
import json
import re
import sys

# metadata file format version
VERSION = "1.0.0"

# hub-specific info
HUB_INFO = {
    "move_hub": {"device-id": 0x40, "checksum-type": "xor"},
    "city_hub": {"device-id": 0x41, "checksum-type": "xor"},
    "cplus_hub": {"device-id": 0x80, "checksum-type": "xor"},
    "prime_hub": {
        "device-id": 0x00,  # FIXME: this is not actual device id
        "checksum-type": "crc32",
    },
}


def generate(fw_version: str, hub_type: str, map_file: io.FileIO, out_file: io.FileIO):
    metadata = {"metadata-version": VERSION, "firmware-version": fw_version}

    if hub_type not in HUB_INFO:
        print("Unknown hub type", file=sys.stderr)
        exit(1)

    metadata.update(HUB_INFO[hub_type])

    flash_origin = None  # Starting address of firmware area in flash memory
    flash_length = None  # Size of firmware area of flash memory
    user_start = None  # Starting address of user .mpy file

    for l in map_file.readlines():
        match = re.match(r"^FLASH\s+(0x[0-9A-Fa-f]{16})\s+(0x[0-9A-Fa-f]{16})", l)
        if match:
            flash_origin = int(match[1], base=0)
            flash_length = int(match[2], base=0)
            continue

        match = re.match(r"^\.user\s+(0x[0-9A-Fa-f]{16})", l)
        if match:
            user_start = int(match[1], base=0)
            continue

    if flash_origin is None:
        print("Failed to find 'FLASH' start address", file=sys.stderr)
        exit(1)

    if flash_length is None:
        print("Failed to find 'FLASH' length", file=sys.stderr)
        exit(1)

    if user_start is None:
        print("Failed to find '.user' start address", file=sys.stderr)
        exit(1)

    metadata["user-mpy-offset"] = user_start - flash_origin
    metadata["max-firmware-size"] = flash_length

    json.dump(metadata, out_file, indent=4, sort_keys=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate firmware metadata.")
    parser.add_argument(
        "fw_version", metavar="<version>", type=str, help="firmware version",
    )
    parser.add_argument(
        "hub_type", metavar="<hub type>", choices=HUB_INFO.keys(), help="hub type",
    )
    parser.add_argument(
        "map_file",
        metavar="<map file>",
        type=argparse.FileType("r"),
        help="firmware linker map file name",
    )
    parser.add_argument(
        "out_file",
        metavar="<output file>",
        type=argparse.FileType("w"),
        help="output file name",
    )

    args = parser.parse_args()

    generate(args.fw_version, args.hub_type, args.map_file, args.out_file)
