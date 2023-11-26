#!/usr/bin/env python3
"""Creates the firmware size data for Pybricks MicroPython"""

import argparse
import os
import subprocess

from azure.core.credentials import AzureNamedKeyCredential
from azure.data.tables import TableClient, UpdateMode
import git

STORAGE_ACCOUNT = os.environ.get("STORAGE_ACCOUNT")
STORAGE_KEY = os.environ.get("STORAGE_KEY")
STORAGE_URL = os.environ.get("STORAGE_URL")
FIRMWARE_SIZE_TABLE = os.environ.get("FIRMWARE_SIZE_TABLE")

PYBRICKS_PATH = os.environ.get("PYBRICKS_PATH", ".")

GITHUB_RUN_NUMBER = os.environ.get("GITHUB_RUN_NUMBER")

parser = argparse.ArgumentParser()
parser.add_argument("hub", metavar="<hub>")
parser.add_argument("commit", metavar="<commit>")
args = parser.parse_args()

pybricks = git.Repo(PYBRICKS_PATH)
assert not pybricks.bare, "Repository not found"

# if credentials were given, connect to remote database
if STORAGE_ACCOUNT:
    firmware_size_table = TableClient(
        STORAGE_URL,
        FIRMWARE_SIZE_TABLE,
        credential=AzureNamedKeyCredential(STORAGE_ACCOUNT, STORAGE_KEY),
    )

if GITHUB_RUN_NUMBER:
    tag = pybricks.git.execute(
        ["git", "describe", "--tags", "--dirty", "--always", "--exclude", "@pybricks/*"]
    )
    os.putenv("MICROPY_GIT_TAG", f"ci-build-{GITHUB_RUN_NUMBER}-{tag}")

# build each commit starting with the oldest
start_commit = (
    subprocess.check_output(["git", "merge-base", "origin/master", args.commit])
    .decode()
    .strip()
)
end_commit = args.commit

for commit in pybricks.iter_commits(
    f"{start_commit}..{end_commit}", ancestry_path=start_commit, reverse=True
):
    print("Checking out", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
    pybricks.git.checkout(commit.hexsha)
    os.putenv("MICROPY_GIT_HASH", commit.hexsha[:8])

    # update only required submodules
    pybricks.git.submodule("update", "--init", "micropython")
    if args.hub in ["cityhub", "movehub", "technichub", "primehub", "essentialhub"]:
        pybricks.submodule("micropython").module().git.submodule(
            "update", "--init", "lib/micropython-lib"
        )
        pybricks.submodule("micropython").module().git.submodule(
            "update", "--init", "lib/stm32lib"
        )
    if args.hub == "primehub" or args.hub == "essentialhub":
        pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
        pybricks.git.submodule("update", "--init", "--checkout", "lib/STM32_USB_Device_Library")

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
    if "firmware_size_table" in globals():
        bin_path = os.path.join(
            PYBRICKS_PATH, "bricks", args.hub, "build", "firmware-base.bin"
        )
        size = os.path.getsize(bin_path)

        firmware_size_table.upsert_entity(
            {
                "PartitionKey": "size",
                "RowKey": commit.hexsha,
                args.hub: size,
            },
            UpdateMode.MERGE,
        )
