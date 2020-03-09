#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

import os
import argparse
import subprocess
from pathlib import Path


BUILD_DIR = "build"
TMP_PY_SCRIPT = "_tmp.py"
TMP_MPY_SCRIPT = "_tmp.mpy"


def make_build_dir():
    # Create build folder if it does not exist
    if not os.path.exists(BUILD_DIR):
        os.mkdir(BUILD_DIR)

    # Raise error if there happens to be a file by this name
    if os.path.isfile(BUILD_DIR):
        raise OSError("A file named build already exists.")


def mpy_bytes_from_file(mpy_cross, path):
    """Compile a Python file with mpy-cross and return as bytes."""

    # Show mpy_cross version
    proc = subprocess.Popen([mpy_cross, "--version"])
    proc.wait()

    # Make the build directory
    make_build_dir()

    # Cross-compile Python file to .mpy and raise errors if any
    mpy_path = os.path.join(BUILD_DIR, Path(path).stem + ".mpy")
    proc = subprocess.run([mpy_cross, path, "-mno-unicode", "-o", mpy_path], check=True)

    # Read the .mpy file and return as bytes
    with open(mpy_path, "rb") as mpy:
        return mpy.read()


def mpy_bytes_from_str(mpy_cross, string):
    """Compile a Python command with mpy-cross and return as bytes."""

    # Make the build directory
    make_build_dir()

    # Path to temporary file
    py_path = os.path.join(BUILD_DIR, TMP_PY_SCRIPT)

    # Write Python command to a file and convert as if it is a regular script.
    with open(py_path, "w") as f:
        f.write(string + "\n")

    # Convert to mpy and get the bytes
    return mpy_bytes_from_file(mpy_cross, py_path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert Python scripts or commands to .mpy bytes."
    )
    parser.add_argument("--mpy_cross", dest="mpy_cross", nargs="?", type=str, required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--file", dest="file", nargs="?", const=1, type=str)
    group.add_argument("--string", dest="string", nargs="?", const=1, type=str)
    args = parser.parse_args()

    if args.file:
        data = mpy_bytes_from_file(args.mpy_cross, args.file)

    if args.string:
        data = mpy_bytes_from_str(args.mpy_cross, args.string)

    # Print as string as a sanity check.
    print("\nBytes:")
    print(data)

    # Print the bytes as a C byte array for development of new MicroPython
    # ports without usable I/O, REPL or otherwise.
    WIDTH = 8
    print(
        "\n// MPY file. Version: {0}. Size: {1}".format(data[1], len(data))
        + "\nconst uint8_t script[] = "
    )
    for i in range(0, len(data), WIDTH):
        chunk = data[i : i + WIDTH]
        hex_repr = ["0x{0}".format(hex(i)[2:].zfill(2).upper()) for i in chunk]
        print("    " + ", ".join(hex_repr) + ",")
    print("};")
