#!/usr/bin/env python3

import importlib.util
import os

# import codeformat from upstream micropython tools
spec = importlib.util.spec_from_file_location(
    "codeformat",
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "tools", "codeformat.py"),
)
codeformat = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codeformat)

# override with Pybricks paths

codeformat.PATHS = [
    "ports/pybricks/bricks/**/*.[ch]",
    "ports/pybricks/extmod/*.[ch]",
    "ports/pybricks/lib/pbio/**/*.[ch]",
    "ports/pybricks/py/*.[ch]",
    # Python
    "ports/pybricks/bricks/**/*.py",
    "ports/pybricks/tests/**/*.py",
    "ports/pybricks/tools/**/*.py",
]

codeformat.EXCLUSIONS = [
    "ports/pybricks/bricks/nxt/nxt-firmware-drivers/**/*.[ch]",
    "ports/pybricks/bricks/*/build*/**/*.[ch]",
]

if __name__ == "__main__":
    codeformat.main()
