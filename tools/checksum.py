#!/usr/bin/env python3
"""Calculate checksum of firmware file in the same way that the bootloader on
the MOVEHUB does."""

from __future__ import print_function

import struct
import sys


def xor_checksum(fw, max_size):
    """Calculate the checksum of a firmware file using a simple xor of every
    four bytes.

    Parameters
    ----------
    fw : file
        The firmware file (a binary buffer - e.g. a file opened in 'rb' mode)
    max_size : int
        The maximum size of the firmware file.

    Returns
    -------
    int
        The checksum
    """
    checksum = 0
    size = 0

    while True:
        word = fw.read(4)
        if not word:
            break
        checksum += struct.unpack('I', word)[0]
        size += 4

    if size > max_size:
        raise ValueError("File is too large")

    for _ in range(size, max_size, 4):
        checksum += 0xffffffff

    checksum &= 0xffffffff
    correction = checksum and (1 << 32) - checksum or 0

    return hex(correction)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Missing arguments", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1], 'rb') as f:
        print(xor_checksum(f, int(sys.argv[2])))
