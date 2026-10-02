# SPDX-License-Identifier: GPL-2.0-only
# Copyright 2006 David Anderson <david.anderson@calixo.net>
# Copyright 2023 The Pybricks Authors

"""
Support for flashing firmware onto a LEGO MINDSTORMS NXT brick.

Based on PyNXT, which implemented the SAM-BA bootloader protocol used by the
AT91SAM7S256. Scans the USB chain for a brick in SAM-BA mode and writes the
firmware to flash.
"""
