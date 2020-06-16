#!/usr/bin/env python3

import importlib.util
import os

# import codeformat from upstream micropython tools
spec = importlib.util.spec_from_file_location(
    "codeformat",
    os.path.join(os.path.dirname(__file__), "..", "micropython", "tools", "codeformat.py"),
)
codeformat = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codeformat)

# override with Pybricks paths

codeformat.PATHS = [
    "bricks/**/*.[ch]",
    "extmod/*.[ch]",
    "lib/pbio/**/*.[ch]",
    "py/*.[ch]",
    # Python
    "bricks/**/*.py",
    "tests/**/*.py",
    "tools/**/*.py",
]

codeformat.EXCLUSIONS = [
    "bricks/nxt/nxt-firmware-drivers/**",
    "bricks/**/build*/**",
    "micropython/**",
]

codeformat.TOP = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

if __name__ == "__main__":
    codeformat.main()
