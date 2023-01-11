#!/usr/bin/env python3

"""
Parses MicroPython .P files for dependencies and collects them in a zip file.

This is used to find what files are actually compiled in firmware for
determining licensing requirements.
"""

import argparse
import pathlib
import typing
import zipfile


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("dir", metavar="<dir>", help="a micropython build directory")
    parser.add_argument("zip", metavar="<zip-file>", help="path to the output zip file")

    args = parser.parse_args()

    # find all of the .P files in the build directory
    p_files = pathlib.Path(args.dir).rglob("*.P")

    all_deps: typing.Set[pathlib.Path] = set()

    # Parse the files. These are technically makefiles but only include
    # targets and dependencies.

    for p in p_files:
        with open(p) as f:
            final_lines: typing.List[str] = []
            current_line: typing.List[str] = []

            for line in f.readlines():
                line = line.strip()

                # lines ending with a \ are continuations
                if line.endswith("\\"):
                    current_line.append(line[:-1])
                else:
                    current_line.append(line)
                    final_lines.append("".join(current_line))
                    current_line.clear()

        # final_lines now contains each line with continuations processed
        # and each line contains a make target and its dependencies.
        for line in final_lines:
            target, deps = line.split(":", 1)

            if not deps:
                continue

            for d in deps.split(" "):
                d = d.strip()

                if not d:
                    continue

                # we need to resolve the paths to ensure uniqueness
                p = pathlib.Path(d)

                if not p.is_absolute():
                    # this assumes the build directory is a subdirectory
                    # of the main makefile
                    p = pathlib.Path(args.dir, "..", p).resolve()

                all_deps.add(p)

    # all_deps now contains all unique dependencies. a copy of each of these
    # is saved in a zip file.

    with zipfile.ZipFile(args.zip, "w") as deps_zip:
        for p in sorted(all_deps):
            deps_zip.write(p, p)
