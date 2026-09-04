#!/usr/bin/env python3
"""Builds a range of commits and records their firmware sizes for one hub.

Builds every commit up to the current HEAD, starting after the newest commit
that already has a recorded size. Results are appended to <hub>.csv in the size
data worktree as one "hash,size" line per commit and committed there. A build
failure stops the script, unless --keep-going records it with an empty size so
that it is never retried and later commits are still built; then only a failure
at HEAD itself is fatal. With --publish, each commit is also pushed to the
GitHub remote, retrying when concurrent CI jobs push in between; the script
fails if a push still does not go through.

Example:

    build-commits.py movehub
    build-commits.py movehub --publish --keep-going
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
    choices=HUBS,
    metavar="<hub>",
    help="hub to build",
)
parser.add_argument(
    "--publish",
    action="store_true",
    help="push each recorded size to the GitHub remote",
)
parser.add_argument(
    "--keep-going",
    action="store_true",
    help="record a failed build and continue; only HEAD has to build",
)
args = parser.parse_args()

pybricks = git.Repo(PYBRICKS_PATH)
assert not pybricks.bare, "Repository not found"

# resolve now, since building checks out each commit in turn
head = pybricks.head.commit.hexsha

size_data = git.Repo(os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR))

CSV_PATH = os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR, f"{args.hub}.csv")


def load_recorded():
    """hash -> size string, where empty string is a recorded build failure"""
    try:
        with open(CSV_PATH, newline="") as f:
            return {row[0]: row[1] for row in csv.reader(f)}
    except FileNotFoundError:
        return {}


recorded = load_recorded()


def record(commit, size):
    """Appends one result and commits it, pushing when enabled."""
    global recorded
    recorded[commit.hexsha] = "" if size is None else str(size)

    message = f"{args.hub}: Add {commit.hexsha[:8]}: "
    if size is None:
        message += "build failed."
    else:
        prev = recorded.get(commit.parents[0].hexsha) if commit.parents else None
        message += f"{size} ({size - int(prev):+d})." if prev else f"{size}."

    for _ in range(10):
        with open(CSV_PATH, "w", newline="") as f:
            csv.writer(f, lineterminator="\n").writerows(recorded.items())

        # nothing left to commit: after a rejected push, the job that beat us
        # may have already published this same commit with the same size
        if not size_data.is_dirty(untracked_files=True):
            return

        size_data.git.add(f"{args.hub}.csv")
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
            merged = load_recorded()
            merged.update(recorded)
            recorded = merged

    sys.exit("Could not push size data; rerun to retry")


# newest ancestor of HEAD already recorded
start = next(
    (c.hexsha for c in pybricks.iter_commits(head) if c.hexsha in recorded),
    None,
)
if start is None:
    sys.exit(f"No recorded ancestor found; seed {SIZE_BRANCH} with a starting commit")

if GITHUB_RUN_NUMBER:
    tag = pybricks.git.execute(
        ["git", "describe", "--tags", "--dirty", "--always", "--exclude", "@pybricks/*"]
    )
    os.putenv("MICROPY_GIT_TAG", f"ci-build-{GITHUB_RUN_NUMBER}-{tag}")


def update_submodules():
    pybricks.git.submodule("update", "--init", "micropython")
    micropython = pybricks.submodule("micropython").module()
    micropython.git.submodule("update", "--init", "lib/micropython-lib")
    micropython.git.submodule("update", "--init", "lib/stm32lib")
    if args.hub in ("primehub", "essentialhub"):
        pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
        pybricks.git.submodule(
            "update", "--init", "--checkout", "lib/STM32_USB_Device_Library"
        )
    if args.hub in ("ev3", "nxt"):
        pybricks.git.submodule("update", "--init", "--checkout", "lib/umm_malloc")
    if args.hub == "buildhat":
        micropython.git.submodule("update", "--init", "lib/pico-sdk")


def build():
    subprocess.check_call(
        [
            "make",
            "-C",
            os.path.join(PYBRICKS_PATH, "bricks", args.hub),
            "build/firmware-base.bin",
            "all",
            "-j",
        ]
    )
    return os.path.getsize(
        os.path.join(PYBRICKS_PATH, "bricks", args.hub, "build", "firmware-base.bin")
    )


# micropython submodule commit that mpy-cross was last built from
mpy_cross_built = None

for commit in pybricks.iter_commits(
    f"{start}..{head}", ancestry_path=True, reverse=True
):
    # recorded results are final: an empty size means the build failed at
    # this commit and would fail again
    if commit.hexsha in recorded:
        print("Skipping", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
        continue

    print("Checking out", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
    pybricks.git.checkout(commit.hexsha)
    os.putenv("MICROPY_GIT_HASH", commit.hexsha[:8])

    update_submodules()

    print("Clean", flush=True)
    # clean the brick directly: top-level clean-<hub> would also clean the
    # mpy-cross that is kept while the micropython submodule is unchanged
    subprocess.check_call(
        ["make", "-C", os.path.join(PYBRICKS_PATH, "bricks", args.hub), "clean"]
    )

    micropython_commit = commit.tree["micropython"].hexsha
    if micropython_commit != mpy_cross_built:
        print("Building mpy-cross", flush=True)
        mpy_cross_path = os.path.join(PYBRICKS_PATH, "micropython", "mpy-cross")
        subprocess.check_call(["make", "-C", mpy_cross_path, "clean"])
        subprocess.check_call(["make", "-C", mpy_cross_path, "CROSS_COMPILE=", "-j"])
        mpy_cross_built = micropython_commit

    print("Building", args.hub, flush=True)
    try:
        size = build()
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print("Build failed:", e, flush=True)
        failure = f'{args.hub} build failed at {commit.hexsha[:8]} "{commit.summary}"'
        if not args.keep_going:
            # deliberately not recorded: keep reporting the failure until the
            # branch is fixed up, which gives its commits new hashes anyway
            sys.exit(f"::error::{failure}")
        print(f"::warning::{failure}", flush=True)
        size = None
    record(commit, size)

# a gap left behind by an accidental bad commit must not fail every later run,
# so once it is recorded only the state of HEAD still matters
if recorded[head] == "":
    sys.exit(f"::error::{args.hub} build failed at HEAD {head[:8]}")
