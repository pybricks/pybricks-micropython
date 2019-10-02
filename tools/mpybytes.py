#!/usr/bin/env python3
from sys import argv
import mpy_cross
import argparse

TMPFILE = 'script.py'


def get_bytes(path):
    """Compile a Python file with mpy-cross and return as list of bytes."""
    # TODO: Check versions and compatibility
    proc = mpy_cross.run(path, '-mcache-lookup-bc')
    proc.wait()

    with open(path[0:-3]+'.mpy', 'rb') as mpy:
        contents = mpy.read()
        return [int(c) for c in contents]


def script_from_str(string):
    with open(TMPFILE, 'w') as f:
        f.write(string + '\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Convert Python scripts or commands to .mpy byte streams.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--file', dest='file', nargs='?', const=1, type=str)
    group.add_argument('--string', dest='string', nargs='?', const=1, type=str)
    args = parser.parse_args()

    if args.file:
        print(get_bytes(args.file))

    if args.string:
        print(get_bytes(TMPFILE))
