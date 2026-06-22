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
        help="the firmware .zip file or a raw .bin file",
    ).completer = FilesCompleter(allowednames=(".zip", ".bin"))

    raw_bin_hub_types = {
        "primehub": HubKind.TECHNIC_LARGE,
        "essentialhub": HubKind.TECHNIC_SMALL,
        "movehub": HubKind.BOOST,
        "cityhub": HubKind.CITY,
        "technic": HubKind.TECHNIC,
        "nxt": HubKind.NXT,
        "ev3": HubKind.EV3,
    }

    parser.add_argument(
        "-k",
        "--hub-kind",
        metavar="<hub-kind>",
        choices=list(raw_bin_hub_types),
        help="the hub kind (only when flashing a raw .bin file); "
        f"one of: {', '.join(raw_bin_hub_types)}",
    )

    parser.add_argument(
        "-n", "--name", metavar="<name>", type=str, help="a custom name for the hub"
    )

    argcomplete.autocomplete(parser)
    args = parser.parse_args()

    logging.basicConfig(
        format="%(asctime)s: %(levelname)s: %(name)s: %(message)s",
        level=logging.DEBUG if args.debug else logging.WARNING,
    )

    # Ask confirmation if flashing a raw .bin file.
    if args.firmware.name.lower().endswith(".bin"):
        if args.hub_kind is None:
            parser.error("--hub-kind is required when flashing a raw .bin file")

        if args.name is not None:
            parser.error("--name is not supported when flashing a raw .bin file")

        hub_kind = raw_bin_hub_types[args.hub_kind]

        print(
            f"Warning: flashing a raw .bin file to a {args.hub_kind} hub. "
            "The file will not be checked for validity."
        )
        answer = input("Are you sure you want to proceed? [y/N] ")
        if answer.strip().lower() not in ("y", "yes"):
            print("Aborted.")
            return

        firmwares = args.firmware.read()
    else:
        if args.hub_kind is not None:
            parser.error("--hub-kind is only supported when flashing a raw .bin file")

        print("Creating firmware...")
        hub_kind, firmwares = create_firmware_blob(args.firmware, args.name)

    # We have a firmware and intended target. Find and flash it.
    if hub_kind in (HubKind.TECHNIC_SMALL, HubKind.TECHNIC_LARGE):
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
