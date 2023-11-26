#!/usr/bin/env python3
"""Builds any missing commits for master branch of Pybricks MicroPython"""

import os
import subprocess
import sys

from azure.core.credentials import AzureNamedKeyCredential
from azure.core.exceptions import ResourceNotFoundError
from azure.data.tables import TableClient, UpdateMode
import git

STORAGE_ACCOUNT = os.environ["STORAGE_ACCOUNT"]
STORAGE_KEY = os.environ["STORAGE_KEY"]
STORAGE_URL = os.environ["STORAGE_URL"]
CI_STATUS_TABLE = os.environ["CI_STATUS_TABLE"]
FIRMWARE_SIZE_TABLE = os.environ["FIRMWARE_SIZE_TABLE"]

PYBRICKS_PATH = os.environ.get("PYBRICKS_PATH", ".")

PYBRICKS_BRANCH = "origin/master"

HUBS = ["movehub", "cityhub", "technichub", "primehub", "essentialhub", "nxt"]

GITHUB_RUN_NUMBER = os.environ.get("GITHUB_RUN_NUMBER")

print("Building commits...")

try:
    pybricks = git.Repo(PYBRICKS_PATH)
except Exception as e:
    print(f"Repository not found at '{PYBRICKS_PATH}':", e)
    print("try setting the PYBRICKS_PATH environment variable")
    sys.exit(1)

assert not pybricks.bare, "Repository not found"

ci_status_table = TableClient(
    STORAGE_URL,
    CI_STATUS_TABLE,
    credential=AzureNamedKeyCredential(STORAGE_ACCOUNT, STORAGE_KEY),
)

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

start_hash = ci_status_table.get_entity("build", "lastHash")["hash"]

if start_hash == pybricks.commit(PYBRICKS_BRANCH).hexsha:
    print("Already up to date.")
    sys.exit(0)


# Process the commits in the tree and log the data
for commit in pybricks.iter_commits(
    f"{start_hash}..{PYBRICKS_BRANCH}", ancestry_path=start_hash, reverse=True
):
    print(f"trying {commit.hexsha}...")

    sizes = {}

    try:
        entity = firmware_size_table.get_entity("size", commit.hexsha)
        # if entity is found but some hubs had null size, redo only those hubs
        hubs = [h for h in HUBS if entity.get(h) is None]
        # save existing sizes since we replace then entity rather than update later
        sizes = {h: int(entity[h]) for h in HUBS if entity.get(h) is not None}
    except ResourceNotFoundError:
        # if there is no entry at all, build all hubs
        hubs = HUBS

    if not hubs:
        print("nothing to do for this commit")
        continue

    # Checkout the Pybricks MicroPython commit for processing
    print("Checking out:", commit.hexsha)
    pybricks.git.checkout(commit.hexsha)
    os.putenv("MICROPY_GIT_HASH", commit.hexsha[:8])

    # update required submodules
    print("Checking out submodules")
    pybricks.git.submodule("update", "--init", "micropython")
    pybricks.submodule("micropython").module().git.submodule(
        "update", "--init", "lib/micropython-lib"
    )
    pybricks.submodule("micropython").module().git.submodule(
        "update", "--init", "lib/stm32lib"
    )
    pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
    pybricks.git.submodule("update", "--init", "--checkout", "lib/STM32_USB_Device_Library")

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

    firmware_size_table.upsert_entity(
        {"PartitionKey": "size", "RowKey": commit.hexsha, **sizes},
        UpdateMode.MERGE,
    )

ci_status_table.update_entity(
    {
        "PartitionKey": "build",
        "RowKey": "lastHash",
        "hash": pybricks.commit(PYBRICKS_BRANCH).hexsha,
    },
    UpdateMode.MERGE,
)
