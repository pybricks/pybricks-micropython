#!/usr/bin/env python3
"""Calculate checksum of firmware file in the same way that the bootloader on
the MOVEHUB does."""

from __future__ import print_function

import struct
import sys


if len(sys.argv) < 3:
    print("Missing arguments", file=sys.stderr)
    sys.exit(1)

checksum = 0
size = 0

with open(sys.argv[1], 'rb') as f:
    while True:
        word = f.read(4)
        if not word:
            break
        checksum += struct.unpack('I', word)[0]
        size += 4

MAX_SIZE = int(sys.argv[2])

if size > MAX_SIZE:
    print("File is too large", file=sys.stderr)
    sys.exit(1)

for _ in range(size, MAX_SIZE, 4):
    checksum += 0xffffffff

checksum &= 0xffffffff
correction = checksum and (1 << 32) - checksum or 0

print(hex(correction))
