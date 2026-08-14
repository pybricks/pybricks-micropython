#!/usr/bin/env python3
"""Builds a range of commits and records their firmware sizes.

Builds every commit up to the current HEAD, starting after the newest commit
that already has a recorded size for all hubs being built. Results are
appended to <hub>.csv in the size data worktree as one "hash,size" line per
commit and committed there. Build failures are recorded with an empty size
so they are never retried. With --publish, each commit is also pushed to the
GitHub remote, retrying when concurrent CI jobs push in between.

Examples:

    build-commits.py movehub --publish   # branch CI matrix job
    build-commits.py --publish           # master stats job, all hubs
"""

import argparse
import csv
import os
import random
import subprocess
import sys
import time

import git

HUBS = [
    "movehub",
    "cityhub",
    "technichub",
    "essentialhub",
    "primehub",
    "nxt",
    "ev3",
    "buildhat",
]

# size-data worktree checked out inside this repo (gitignored), same as CI
SIZE_DATA_DIR = "size-data"
SIZE_BRANCH = "size-data"

PYBRICKS_PATH = os.environ.get("PYBRICKS_PATH", ".")
GITHUB_RUN_NUMBER = os.environ.get("GITHUB_RUN_NUMBER")

parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
)
parser.add_argument(
    "hub",
    nargs="?",
    choices=HUBS,
    metavar="<hub>",
    help="build only this hub; default is all hubs",
)
parser.add_argument(
    "--publish",
    action="store_true",
    help="push each recorded size to the GitHub remote",
)
args = parser.parse_args()

hubs = [args.hub] if args.hub else HUBS

pybricks = git.Repo(PYBRICKS_PATH)
assert not pybricks.bare, "Repository not found"

# resolve now, since building checks out each commit in turn
head = pybricks.head.commit.hexsha

size_data = git.Repo(os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR))


def csv_path(hub):
    return os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR, f"{hub}.csv")


def load_recorded(hub):
    """hash -> size string, where empty string is a recorded build failure"""
    try:
        with open(csv_path(hub), newline="") as f:
            return {row[0]: row[1] for row in csv.reader(f)}
    except FileNotFoundError:
        return {}


recorded = {h: load_recorded(h) for h in hubs}


def record(hub, commit, size):
    """Appends one result and commits it, pushing when enabled."""
    recorded[hub][commit.hexsha] = "" if size is None else str(size)

    message = f"{hub}: Add {commit.hexsha[:8]}: "
    if size is None:
        message += "build failed."
    else:
        prev = recorded[hub].get(commit.parents[0].hexsha) if commit.parents else None
        message += f"{size} ({size - int(prev):+d})." if prev else f"{size}."

    for _ in range(10):
        with open(csv_path(hub), "w", newline="") as f:
            csv.writer(f, lineterminator="\n").writerows(recorded[hub].items())

        # another job may have already recorded the same result
        if not size_data.is_dirty(untracked_files=True):
            return

        size_data.git.add(f"{hub}.csv")
        size_data.git.commit("-m", message)

        if not args.publish:
            return

        try:
            size_data.git.push()
            return
        except git.GitCommandError:
            # another CI job pushed in between: redo on top of its result,
            # waiting a random time to spread out the contending jobs
            time.sleep(random.uniform(1, 10))
            size_data.git.fetch()
            size_data.git.reset("--hard", f"origin/{SIZE_BRANCH}")
            merged = load_recorded(hub)
            merged.update(recorded[hub])
            recorded[hub] = merged

    # not fatal: the result rides along with the next push, or is rebuilt later
    print("Could not push size data, continuing", flush=True)


# newest ancestor of HEAD already recorded for all hubs being built
start = next(
    (
        c.hexsha
        for c in pybricks.iter_commits(head)
        if all(c.hexsha in recorded[h] for h in hubs)
    ),
    None,
)
if start is None:
    sys.exit(f"No recorded ancestor found; seed {SIZE_BRANCH} with a starting commit")

if GITHUB_RUN_NUMBER:
    tag = pybricks.git.execute(
        ["git", "describe", "--tags", "--dirty", "--always", "--exclude", "@pybricks/*"]
    )
    os.putenv("MICROPY_GIT_TAG", f"ci-build-{GITHUB_RUN_NUMBER}-{tag}")


def update_submodules(hubs):
    pybricks.git.submodule("update", "--init", "micropython")
    micropython = pybricks.submodule("micropython").module()
    micropython.git.submodule("update", "--init", "lib/micropython-lib")
    micropython.git.submodule("update", "--init", "lib/stm32lib")
    if "primehub" in hubs or "essentialhub" in hubs:
        pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
        pybricks.git.submodule(
            "update", "--init", "--checkout", "lib/STM32_USB_Device_Library"
        )
    if "ev3" in hubs or "nxt" in hubs:
        pybricks.git.submodule("update", "--init", "--checkout", "lib/umm_malloc")
    if "buildhat" in hubs:
        micropython.git.submodule("update", "--init", "lib/pico-sdk")


def build(hub):
    subprocess.check_call(
        [
            "make",
            "-C",
            os.path.join(PYBRICKS_PATH, "bricks", hub),
            "build/firmware-base.bin",
            "all",
            "-j",
        ]
    )
    return os.path.getsize(
        os.path.join(PYBRICKS_PATH, "bricks", hub, "build", "firmware-base.bin")
    )


failures = False

for commit in pybricks.iter_commits(
    f"{start}..{head}", ancestry_path=True, reverse=True
):
    # recorded results are final: an empty size means the build failed at
    # this commit and would fail again
    todo = [h for h in hubs if commit.hexsha not in recorded[h]]
    if not todo:
        print("Skipping", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
        continue

    print("Checking out", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
    pybricks.git.checkout(commit.hexsha)
    os.putenv("MICROPY_GIT_HASH", commit.hexsha[:8])

    update_submodules(todo)

    print("Clean all", flush=True)
    subprocess.check_call(["make", "-C", PYBRICKS_PATH, "clean-all"])

    print("Building mpy-cross", flush=True)
    mpy_cross_path = os.path.join(PYBRICKS_PATH, "micropython", "mpy-cross")
    subprocess.check_call(["make", "-C", mpy_cross_path, "CROSS_COMPILE=", "-j"])

    for hub in todo:
        print("Building", hub, flush=True)
        try:
            size = build(hub)
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            print("Build failed:", e, flush=True)
            size = None
            failures = True
        record(hub, commit, size)

if failures:
    sys.exit("Some builds failed")
