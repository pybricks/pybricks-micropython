# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2024 The Pybricks Authors

"""Command line tool to flash firmware on a LEGO Powered Up device."""

import argparse
import asyncio
import logging
import sys
from os import path

import argcomplete
from argcomplete.completers import FilesCompleter

from dfu import flash_dfu
from firmware import create_firmware_blob
from lwp3.bytecodes import HubKind
from ev3 import flash_ev3
from nxt.flash import flash_nxt

from lwp3.flash import flash_ble

PROG_NAME = path.basename(sys.argv[0])


def main():
    """Runs the firmware flashing command line interface."""

    parser = argparse.ArgumentParser(
        prog=PROG_NAME,
        description="Flash firmware on a LEGO Powered Up device.",
    )

    parser.add_argument(
        "-d", "--debug", action="store_true", help="enable debug logging"
    )

    parser.add_argument(
        "firmware",
        metavar="<firmware-file>",
        type=argparse.FileType(mode="rb"),
        help="the firmware .zip file",
    ).completer = FilesCompleter(allowednames=(".zip",))

    parser.add_argument(
        "-n", "--name", metavar="<name>", type=str, help="a custom name for the hub"
    )

    argcomplete.autocomplete(parser)
    args = parser.parse_args()

    logging.basicConfig(
        format="%(asctime)s: %(levelname)s: %(name)s: %(message)s",
        level=logging.DEBUG if args.debug else logging.WARNING,
    )

    print("Creating firmware...")

    firmware, metadata, license = create_firmware_blob(args.firmware, args.name)
    hub_kind = HubKind(metadata["device-id"])

    if hub_kind in (HubKind.TECHNIC_SMALL, HubKind.TECHNIC_LARGE):
        flash_dfu(firmware, metadata)
    elif hub_kind in [HubKind.BOOST, HubKind.CITY, HubKind.TECHNIC]:
        asyncio.run(flash_ble(hub_kind, firmware, metadata))
    elif hub_kind == HubKind.NXT:
        flash_nxt(firmware)
    elif hub_kind == HubKind.EV3:
        flash_ev3(firmware)
    else:
        raise ValueError(f"unsupported hub kind: {hub_kind}")


if __name__ == "__main__":
    main()
