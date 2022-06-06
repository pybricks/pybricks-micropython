# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

# This script takes the OBJ defined in a typical Makefile, and converts them
# to a format understood by the EV3RT build system. See app.cfg for the
# expected output syntax.

import sys
from pathlib import Path

# First argument is destination file.
config_path = Path(sys.argv[1])

# Remaining args are the OBJ coming from the Makefile.
objects = (Path(p) for p in sys.argv[2:])

# Add all dependencies in the expected format for the EV3RT build system.
with open(config_path, "w") as config_file:
    config_file.write('#include "api.cfg.h"\n')
    for obj in objects:
        config_file.write(f'ATT_MOD("{obj}");\n')
