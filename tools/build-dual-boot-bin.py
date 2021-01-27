#!/usr/bin/env python3

import argparse
import os
import sys
from io import BytesIO, FileIO
from pathlib import Path

try:
    # This will fail when this file is run as a script
    from .checksum import crc32_checksum
except ImportError:
    # In that case, we have to modify sys.path to get the import
    sys.path.insert(0, Path(__file__).parent.resolve())
    from checksum import crc32_checksum


# These values are for SPIKE Prime. Also see prime_hub.ld.
FLASH_SIZE = 1024 * 1024  # 1MiB
BIN1_BASE_OFFSET = 0x8000
BIN2_BASE_OFFSET = 0xC0000


def build_blob(bin1: FileIO, bin2: FileIO, out: FileIO) -> None:
    """Combines two firmware binary blobs ``bin1`` and ``bin2`` into a single
    firmware blob and saves them in ``out``.
    """
    bin1_size = os.fstat(bin1.fileno()).st_size
    bin2_size = os.fstat(bin2.fileno()).st_size
    bin2_offset = BIN2_BASE_OFFSET - BIN1_BASE_OFFSET
    size = bin2_offset + bin2_size
    max_size = FLASH_SIZE - BIN1_BASE_OFFSET

    if bin1_size >= bin2_offset:
        raise ValueError(f"{bin1.name} is too big!")
    if size >= max_size:
        raise ValueError(f"{bin2.name} is too big!")

    # Create a new combined firmware blob
    blob = memoryview(bytearray(size))
    bin1.readinto(blob)
    bin2.readinto(blob[bin2_offset:])

    # Read Reset_Handler pointers in vector tables.
    bin1_reset_handler = blob[4:8].tobytes()
    bin2_reset_handler = blob[bin2_offset + 4 : bin2_offset + 8].tobytes()

    # Swap Reset_Handler pointers. This will cause the second
    # firmware to boot first.
    blob[4:8] = bin2_reset_handler
    blob[bin2_offset + 4 : bin2_offset + 8] = bin1_reset_handler

    # The final checksum is for the entire new blob
    # This overrides the checksum of the second firmware.
    blob[-4:] = crc32_checksum(BytesIO(blob), max_size).to_bytes(4, "little")

    out.write(blob)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "bin1",
        metavar="<input firmware file 1>",
        type=argparse.FileType("rb"),
        help="the official LEGO firmware binary blob",
    )
    parser.add_argument(
        "bin2",
        metavar="<intput firmware file 2>",
        type=argparse.FileType("rb"),
        help="the Pybricks firmware-dual-boot-base.bin binary blob",
    )
    parser.add_argument(
        "out",
        metavar="<output firmware file>",
        type=argparse.FileType("wb"),
        help="the output file",
    )
    args = parser.parse_args()

    build_blob(args.bin1, args.bin2, args.out)
