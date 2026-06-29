# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2024 The Pybricks Authors

"""Command line tool to flash firmware on a LEGO Powered Up device."""

import argcomplete
import argparse
import asyncio
import logging
import sys
import time

from argcomplete.completers import FilesCompleter
from os import path

from dfu import flash_dfu
from firmware import create_firmware_blob
from lwp3.bytecodes import HubKind
from ev3 import flash_ev3
from nxt.flash import flash_nxt
from lwp3.flash import flash_ble
from serdev.connect import get_serial_device
from serdev.pybricks import reboot_to_update_mode_pybricks
from serdev.spike import reboot_for_update_spike_prime

PROG_NAME = path.basename(sys.argv[0])


def main():
    """Runs the firmware flashing command line interface."""

    parser = argparse.ArgumentParser(
        prog=PROG_NAME,
        description="Flash firmware on a Pybricks compatible device.",
    )

    parser.add_argument(
        "-d", "--debug", action="store_true", help="enable debug logging"
    )

    parser.add_argument(
        "firmware",
        metavar="<firmware-file>",
        type=argparse.FileType(mode="rb"),
        help="the firmware .zip file or a raw .bin file",
    ).completer = FilesCompleter(allowednames=(".zip", ".bin"))

    parser.add_argument(
        "-n", "--name", metavar="<name>", type=str, help="a custom name for the hub"
    )

    argcomplete.autocomplete(parser)
    args = parser.parse_args()

    logging.basicConfig(
        format="%(asctime)s: %(levelname)s: %(name)s: %(message)s",
        level=logging.DEBUG if args.debug else logging.WARNING,
    )

    print("Unpacking firmware.")
    hub_kind, firmwares = create_firmware_blob(args.firmware, args.name)

    # We have a firmware and intended target. Find and flash it.
    if hub_kind in (HubKind.TECHNIC_SMALL, HubKind.TECHNIC_LARGE):
        # Newer SPIKE Prime hubs can automatically reboot into DFU mode.
        if hub_kind == HubKind.TECHNIC_LARGE:
            serial = get_serial_device()
            if serial:
                print("Found serial device. Look for official Spike firmware.")
                rebooted_spike_firmware = reboot_for_update_spike_prime(serial)
                if not rebooted_spike_firmware:
                    print("Looking for Pybricks firmware instead.")
                    reboot_to_update_mode_pybricks(serial)
                # Give the hub some time to reboot into DFU.
                print("Waiting for hub to reboot into DFU mode.")
                time.sleep(1.5)
        # Flash the firmware using DFU.
        flash_dfu(firmwares, hub_kind)
    elif hub_kind in [HubKind.BOOST, HubKind.CITY, HubKind.TECHNIC]:
        asyncio.run(flash_ble(firmwares, hub_kind))
    elif hub_kind == HubKind.NXT:
        flash_nxt(firmwares)
    elif hub_kind == HubKind.EV3:
        flash_ev3(firmwares)
    else:
        raise ValueError(f"unsupported hub kind: {hub_kind}")


if __name__ == "__main__":
    main()
