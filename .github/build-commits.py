#!/usr/bin/env python3
"""Builds a range of commits and records their firmware sizes for one hub.

Builds every commit up to the current HEAD, starting after the newest commit
that already has a recorded size. Results are appended to <build>.csv in the
size data worktree as one "hash,size" line per commit and committed there,
where <build> is the hub itself for all but the Prime Hub, which ships two
variants and so has one file per variant. A build failure stops the script,
unless --keep-going records it with an empty size so that it is never retried
and later commits are still built; then only a failure at HEAD itself is fatal.
With --publish, each commit is also pushed to the GitHub remote, retrying when
concurrent CI jobs push in between; the script fails if a push still does not
go through.

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

# Firmware builds recorded for each hub, each with its own file of sizes. Only
# the Prime Hub has more than one: it ships as two variants in a single
# firmware.zip, one for each of the two hardware revisions.
VARIANTS = {
    "primehub": ["primehub_f4", "primehub_h5"],
}

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

variants = VARIANTS.get(args.hub, [args.hub])


def csv_path(variant):
    return os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR, f"{variant}.csv")


def load_recorded(variant):
    """hash -> size string, where empty string is a recorded build failure"""
    try:
        with open(csv_path(variant), newline="") as f:
            return {row[0]: row[1] for row in csv.reader(f)}
    except FileNotFoundError:
        return {}


recorded = {v: load_recorded(v) for v in variants}


def record(variant, commit, size):
    """Appends one result and commits it, pushing when enabled."""
    recorded[variant][commit.hexsha] = "" if size is None else str(size)

    message = f"{variant}: Add {commit.hexsha[:8]}: "
    if size is None:
        message += "build failed."
    else:
        prev = (
            recorded[variant].get(commit.parents[0].hexsha) if commit.parents else None
        )
        message += f"{size} ({size - int(prev):+d})." if prev else f"{size}."

    for _ in range(10):
        with open(csv_path(variant), "w", newline="") as f:
            csv.writer(f, lineterminator="\n").writerows(recorded[variant].items())

        # nothing left to commit: after a rejected push, the job that beat us
        # may have already published this same commit with the same size
        if not size_data.is_dirty(untracked_files=True):
            return

        size_data.git.add(f"{variant}.csv")
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
            # the other job's results for every build, not just this one, or
            # the next write would drop them again
            for other in variants:
                merged = load_recorded(other)
                merged.update(recorded[other])
                recorded[other] = merged

    sys.exit("Could not push size data; rerun to retry")


# Newest ancestor of HEAD already recorded, per build, since a variant added
# later has a much shorter history than the hub it belongs to. Walking from
# HEAD, the last one found is the oldest, which is where the build loop starts.
starts = {}
start = None

for commit in pybricks.iter_commits(head):
    for variant in variants:
        if variant not in starts and commit.hexsha in recorded[variant]:
            starts[variant] = start = commit.hexsha

    if len(starts) == len(variants):
        break

unseeded = [v for v in variants if v not in starts]
if unseeded:
    sys.exit(
        f"No recorded ancestor found for {', '.join(unseeded)};"
        f" seed {SIZE_BRANCH} with a starting commit"
    )

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
    if any(sm.path == "lib/CMSIS_6" for sm in micropython.submodules):
        # only present since MicroPython v1.29, older commits use lib/cmsis
        micropython.git.submodule("update", "--init", "lib/CMSIS_6")
    if args.hub in ("primehub", "essentialhub"):
        pybricks.git.submodule("update", "--init", "--checkout", "lib/btstack")
        pybricks.git.submodule(
            "update", "--init", "--checkout", "lib/STM32_USB_Device_Library"
        )
    if args.hub in ("ev3", "nxt"):
        pybricks.git.submodule("update", "--init", "--checkout", "lib/umm_malloc")
    if args.hub == "buildhat":
        micropython.git.submodule("update", "--init", "lib/pico-sdk")


def brick_dir(variant):
    return os.path.join(PYBRICKS_PATH, "bricks", variant)


def build(variant):
    subprocess.check_call(
        [
            "make",
            "-C",
            brick_dir(variant),
            "build/firmware-base.bin",
            "all",
            "-j",
        ]
    )
    return os.path.getsize(
        os.path.join(brick_dir(variant), "build", "firmware-base.bin")
    )


# micropython submodule commit that mpy-cross was last built from
mpy_cross_built = None

for commit in pybricks.iter_commits(
    f"{start}..{head}", ancestry_path=True, reverse=True
):
    # recorded results are final: an empty size means the build failed at
    # this commit and would fail again
    todo = [v for v in variants if commit.hexsha not in recorded[v]]
    if not todo:
        print("Skipping", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
        continue

    print("Checking out", commit.hexsha[:8], f'"{commit.summary}"', flush=True)
    pybricks.git.checkout(commit.hexsha)
    os.putenv("MICROPY_GIT_HASH", commit.hexsha[:8])

    update_submodules()

    print("Clean", flush=True)
    for variant in todo:
        # clean the brick directly: top-level clean-<hub> would also clean the
        # mpy-cross that is kept while the micropython submodule is unchanged
        subprocess.check_call(["make", "-C", brick_dir(variant), "clean"])

    micropython_commit = commit.tree["micropython"].hexsha
    if micropython_commit != mpy_cross_built:
        print("Building mpy-cross", flush=True)
        mpy_cross_path = os.path.join(PYBRICKS_PATH, "micropython", "mpy-cross")
        subprocess.check_call(["make", "-C", mpy_cross_path, "clean"])
        subprocess.check_call(["make", "-C", mpy_cross_path, "CROSS_COMPILE=", "-j"])
        mpy_cross_built = micropython_commit

    for variant in todo:
        print("Building", variant, flush=True)
        try:
            size = build(variant)
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            print("Build failed:", e, flush=True)
            failure = (
                f'{variant} build failed at {commit.hexsha[:8]} "{commit.summary}"'
            )
            if not args.keep_going:
                # deliberately not recorded: keep reporting the failure until
                # the branch is fixed up, which gives its commits new hashes
                sys.exit(f"::error::{failure}")
            print(f"::warning::{failure}", flush=True)
            size = None
        record(variant, commit, size)

# a gap left behind by an accidental bad commit must not fail every later run,
# so once it is recorded only the state of HEAD still matters
for variant in variants:
    if recorded[variant][head] == "":
        sys.exit(f"::error::{variant} build failed at HEAD {head[:8]}")
