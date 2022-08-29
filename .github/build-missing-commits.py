#!/usr/bin/env python3
"""Builds any missing commits for master branch of Pybricks MicroPython"""

import os
import subprocess
import sys

import git

from azure.common import AzureMissingResourceHttpError
from azure.cosmosdb.table.tableservice import TableService

STORAGE_ACCOUNT = os.environ["STORAGE_ACCOUNT"]
STORAGE_KEY = os.environ["STORAGE_KEY"]
CI_STATUS_TABLE = os.environ["CI_STATUS_TABLE"]
FIRMWARE_SIZE_TABLE = os.environ["FIRMWARE_SIZE_TABLE"]

PYBRICKS_PATH = os.environ.get("PYBRICKS_PATH", ".")

PYBRICKS_BRANCH = "origin/master"

HUBS = ["movehub", "cityhub", "technichub", "primehub", "essentialhub", "nxt"]

print("Building commits...")

try:
    pybricks = git.Repo(PYBRICKS_PATH)
except Exception as e:
    print(f"Repository not found at '{PYBRICKS_PATH}':", e)
    print("try setting the PYBRICKS_PATH environment variable")
    sys.exit(1)

assert not pybricks.bare, "Repository not found"

service = TableService(STORAGE_ACCOUNT, STORAGE_KEY)

start_hash = service.get_entity(CI_STATUS_TABLE, "build", "lastHash")["hash"]

if start_hash == pybricks.commit(PYBRICKS_BRANCH).hexsha:
    print("Already up to date.")
    sys.exit(0)


# Process the commits in the tree and log the data
for commit in pybricks.iter_commits(f"{start_hash}..{PYBRICKS_BRANCH}"):
    print(f"trying {commit.hexsha}...")

    sizes = {}

    try:
        entity = service.get_entity(FIRMWARE_SIZE_TABLE, "size", commit.hexsha)
        # if entity is found but some hubs had null size, redo only those hubs
        hubs = [h for h in HUBS if entity.get(h) is None]
        # save existing sizes since we replace then entity rather than update later
        sizes = {h: int(entity.get(h)) for h in HUBS if entity.get(h) is not None}
    except AzureMissingResourceHttpError:
        # if there is no entry at all, build all hubs
        hubs = HUBS

    if not hubs:
        print("nothing to do for this commit")
        continue

    # Checkout the Pybricks MicroPython commit for processing
    print("Checking out:", commit.hexsha)
    pybricks.git.checkout(commit.hexsha)

    # update required submodules
    print("Checking out submodules")
    pybricks.git.submodule("update", "--init", "micropython")
    pybricks.submodule("micropython").module().git.submodule(
        "update", "--init", "lib/stm32lib"
    )
    pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
    pybricks.git.submodule(
        "update", "--init", "--checkout", "bricks/nxt/nxt-firmware-drivers"
    )

    # Make mpy-cross once
    print("Building mpy-cross")
    mpy_cross_path = os.path.join(PYBRICKS_PATH, "micropython", "mpy-cross")
    subprocess.check_call(["make", "-C", mpy_cross_path, "CROSS_COMPILE="])

    # Make the targets simultaneously
    print("Building firmware")
    procs = {
        target: subprocess.Popen(
            [
                "make",
                "-C",
                os.path.join(PYBRICKS_PATH, "bricks", target),
                "clean",
                "build/firmware-base.bin",
            ]
        )
        for target in hubs
    }

    # Get target sizes
    for target, proc in procs.items():
        # Wait for the target to complete building or fail
        proc.wait()

        # Get build size on success
        bin_path = os.path.join(
            PYBRICKS_PATH, "bricks", target, "build", "firmware-base.bin"
        )
        try:
            sizes[target] = os.path.getsize(bin_path)
        except FileNotFoundError:
            pass

    service.insert_or_replace_entity(
        FIRMWARE_SIZE_TABLE, {"PartitionKey": "size", "RowKey": commit.hexsha, **sizes}
    )

service.update_entity(
    CI_STATUS_TABLE,
    {
        "PartitionKey": "build",
        "RowKey": "lastHash",
        "hash": pybricks.commit(PYBRICKS_BRANCH).hexsha,
    },
)
