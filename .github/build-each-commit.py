#!/usr/bin/env python3
"""Creates the firmware size data for Pybricks MicroPython"""

import argparse
import os
import subprocess

import git

from azure.cosmosdb.table.tableservice import TableService

STORAGE_ACCOUNT = os.environ.get("STORAGE_ACCOUNT")
STORAGE_KEY = os.environ.get("STORAGE_KEY")
FIRMWARE_SIZE_TABLE = os.environ.get("FIRMWARE_SIZE_TABLE")

PYBRICKS_PATH = os.environ.get("PYBRICKS_PATH", ".")


parser = argparse.ArgumentParser()
parser.add_argument("hub", metavar="<hub>")
parser.add_argument("commit", metavar="<commit>")
args = parser.parse_args()

pybricks = git.Repo(PYBRICKS_PATH)
assert not pybricks.bare, "Repository not found"

# if credentials were given, connect to remote database
if STORAGE_ACCOUNT:
    service = TableService(STORAGE_ACCOUNT, STORAGE_KEY)

# build each commit starting with the oldest
start_commit = (
    subprocess.check_output(["git", "merge-base", "origin/master", args.commit])
    .decode()
    .strip()
)
end_commit = args.commit

for commit in reversed(list(pybricks.iter_commits(f"{start_commit}..{end_commit}"))):
    print("Checking out", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
    pybricks.git.checkout(commit.hexsha)

    # update only required submodules
    pybricks.git.submodule("update", "--init", "micropython")
    pybricks.git.submodule("update", "--init", "lib/libfixmath")
    if args.hub in ["cityhub", "movehub", "technichub", "primehub", "essentialhub"]:
        pybricks.submodule("micropython").module().git.submodule(
            "update", "--init", "lib/stm32lib"
        )
    if args.hub == "primehub" or args.hub == "essentialhub":
        pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
    if args.hub == "nxt":
        pybricks.git.submodule(
            "update", "--init", "--checkout", "bricks/nxt/nxt-firmware-drivers"
        )

    # build the firmware
    subprocess.check_call(
        [
            "make",
            "-C",
            os.path.join(PYBRICKS_PATH, "bricks", args.hub),
            "clean",
            "build/firmware-base.bin",
            "all",
        ]
    )

    # upload firmware size
    if "service" in globals():
        bin_path = os.path.join(
            PYBRICKS_PATH, "bricks", args.hub, "build", "firmware-base.bin"
        )
        size = os.path.getsize(bin_path)

        service.insert_or_merge_entity(
            FIRMWARE_SIZE_TABLE,
            {
                "PartitionKey": "size",
                "RowKey": commit.hexsha,
                args.hub: size,
            },
        )
