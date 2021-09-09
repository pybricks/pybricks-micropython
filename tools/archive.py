#!/usr/bin/env python3

"""Create source code archive, including submodules, for binary releases."""

import argparse
import os
import pathlib
import subprocess
import tempfile
from typing import Optional


TOP_DIR = pathlib.Path(__file__).parent.parent


def git_archive(directory: Optional[os.PathLike], out: os.PathLike) -> None:
    """Calls the ``git archive`` command line.

    Arguments:
        directory: The subdirectory
        out: The output file path.
    """
    args = ["git", "archive", f"--output={out}"]
    cwd = TOP_DIR
    if directory:
        cwd = cwd.joinpath(directory)
        args.append(f"--prefix={directory}/")
    args.append("HEAD")

    subprocess.check_call(args, cwd=cwd)


def archive(hub: str) -> None:
    """Build source code archive from git tree for specific hub.

    Only git submodules used by ``hub`` will be included.
    """
    with tempfile.TemporaryDirectory(prefix="pybricks-micropython-archive-") as d:
        archives = []

        # main pybricks-micropython source code
        main_tar = pathlib.Path(d, "pybricks-micropython.tar")
        git_archive(None, main_tar)
        archives.append(main_tar)

        # every hub includes micropython/
        micropython_tar = pathlib.Path(d, "micropython.tar")
        git_archive("micropython", micropython_tar)
        archives.append(micropython_tar)

        # only STM32 hubs have stm32lib
        if hub in ["cityhub", "essentialhub", "movehub", "primehub", "technichub"]:
            stm32lib_tar = pathlib.Path(d, "stm32lib.tar")
            git_archive("micropython/lib/stm32lib", stm32lib_tar)
            archives.append(stm32lib_tar)

        # every hub includes libfixmath
        libfixmath_tar = pathlib.Path(d, "libfixmath.tar")
        git_archive("lib/libfixmath", libfixmath_tar)
        archives.append(libfixmath_tar)

        # extra library for SPIKE hubs
        if hub == "primehub" or hub == "essentialhub":
            btstack_tar = pathlib.Path(d, "btstack.tar")
            git_archive("lib/btstack", btstack_tar)
            archives.append(btstack_tar)

        # extra library for NXT
        if hub == "nxt":
            nxt_tar = pathlib.Path(d, "nxt.tar")
            git_archive("bricks/nxt/nxt-firmware-drivers", nxt_tar)
            archives.append(nxt_tar)

        # merge all of the individual tar files
        final_tar = pathlib.Path(f"pybricks-micropython-{hub}.tar.gz")
        final_tar.unlink(missing_ok=True)
        for a in archives:
            subprocess.check_call(["tar", "--concatenate", f"--file={final_tar}", a])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("hub", metavar="<hub>", help="The type of hub")
    args = parser.parse_args()
    archive(args.hub)
