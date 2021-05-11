#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2021 The Pybricks Authors

"""Build files for the dual boot installer for SPIKE Prime."""

import os
import hashlib
import shutil

BUILD_PATH = "build"
BRICK_PATH = "."

# Read the Pybricks firmware.
with open(os.path.join(BUILD_PATH, "firmware-dual-boot-base.bin"), "rb") as f:
    firmware_hash = hashlib.sha256(f.read()).hexdigest()

# Read the install script.
with open(os.path.join(BRICK_PATH, "install_pybricks.py"), "rb") as f:
    install_hash = hashlib.sha256(f.read()).hexdigest()

# Write the hash file used to verify upload integrity.
with open(os.path.join(BUILD_PATH, "install_pybricks_hash.txt"), "w") as f:
    f.writelines([firmware_hash + "\n", install_hash + "\n"])

# Copy install script to build folder.
shutil.copyfile(
    os.path.join(BRICK_PATH, "install_pybricks.py"),
    os.path.join(BUILD_PATH, "install_pybricks.py"),
)
