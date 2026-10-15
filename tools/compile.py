#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Compiles a Pybricks script and the local modules it imports, and writes the
result to stdout in the multi-mpy format that hubs receive from a code editor.

This is used by the virtual hub to load a program as if it had been downloaded.
It uses the mpy-cross built in this repository, so the ABI always matches.
"""

import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

PBTOP = pathlib.Path(__file__).resolve().parent.parent
MPY_CROSS = PBTOP / "micropython" / "mpy-cross" / "build" / "mpy-cross"
MPY_TOOL = PBTOP / "micropython" / "tools" / "mpy-tool.py"


def compile_module(proj_dir, module_name, build_dir):
    """
    Compiles one module and gets the names of the modules it imports.

    Returns ``None`` if the module is not in the project directory, which means
    it is built into the firmware instead.
    """
    parts = module_name.split(".")

    # A relative import has an empty module name. The hub rejects those at
    # runtime, so there is nothing to look up here.
    if not all(parts):
        return None

    base = pathlib.Path(*parts)

    for source in (base.with_suffix(".py"), base / "__init__.py"):
        if (proj_dir / source).is_file():
            break
    else:
        return None

    mpy_path = build_dir / (module_name + ".mpy")

    if subprocess.run(
        [MPY_CROSS, "-o", mpy_path, "-s", str(source), proj_dir / source]
    ).returncode:
        sys.exit(1)

    # mpy-cross does not say what the module imports, so disassemble the result
    # and look for the import opcodes.
    info = json.loads(
        subprocess.check_output(
            [sys.executable, MPY_TOOL, "--disassemble", "--json", mpy_path],
            stderr=subprocess.DEVNULL,
        )
    )
    imports = {
        asm["text"].split(" ")[-1]
        for asm in info["asm"]
        if asm.get("disassembly") == "IMPORT_NAME"
    }

    return mpy_path.read_bytes(), imports


def compile_multi_file(path, build_dir):
    """
    Compiles the script at ``path`` along with the modules it imports from the
    same directory, recursively.

    The result is the concatenation of, for each module, its size as a little
    endian uint32, its zero terminated name, and its mpy data. The main script
    is included as ``__main__``.
    """
    proj_dir = path.parent

    mpy, imports = compile_module(proj_dir, path.stem, build_dir)
    modules = {"__main__": mpy}
    builtins = set()

    # Modules may import other modules, so keep going until nothing is left.
    while todo := imports - builtins - modules.keys():
        for name in sorted(todo):
            result = compile_module(proj_dir, name, build_dir)
            if result is None:
                builtins.add(name)
                continue
            modules[name], new_imports = result
            imports |= new_imports

    parts = []
    for name, mpy in modules.items():
        parts.append(len(mpy).to_bytes(4, "little"))
        parts.append(name.encode() + b"\x00")
        parts.append(mpy)

    return b"".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=pathlib.Path, help="Python script to compile.")
    args = parser.parse_args()

    if not MPY_CROSS.is_file():
        sys.exit(f"{MPY_CROSS} not found. Run `make mpy-cross` first.")

    with tempfile.TemporaryDirectory() as build_dir:
        mpy = compile_multi_file(args.file.resolve(), pathlib.Path(build_dir))

    sys.stdout.buffer.write(mpy)


if __name__ == "__main__":
    main()
