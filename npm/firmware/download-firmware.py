#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""Script to download Pybricks firmware from GitHub into the build/ directory."""

import json
import os
import pathlib
import re
import sys
import urllib

from github import Github
from github.GithubException import (
    BadCredentialsException,
    RateLimitExceededException,
    UnknownObjectException,
)


def exit_with_error(*args):
    print("Error:", *args, file=sys.stderr)
    sys.exit(1)


def download_firmware():
    print("Fetching git tag from version build metadata in package.json...", end=" ")

    package_json = pathlib.Path(__file__).parent.joinpath("package.json").absolute()
    with open(package_json) as f:
        tag = re.sub(r".*\+firmware\.(.*)", "\\1", json.load(f)["version"])

    print(tag)

    try:
        github = Github(os.environ.get("GITHUB_TOKEN"))
    except BadCredentialsException:
        exit_with_error("Invalid credentials.")

    try:
        repo = github.get_repo("pybricks/pybricks-micropython")
        try:
            release = repo.get_release(tag)
        except UnknownObjectException:
            exit_with_error(f"No release for tag {tag}")

        build_dir = pathlib.Path(__file__).parent.joinpath("build").absolute()
        try:
            os.mkdir(build_dir)
        except OSError:
            pass

        for asset in release.get_assets():
            short_name = re.sub(r"pybricks-(.*)-v.*(\.zip)", "\\1\\2", asset.name)

            print(f"Downloading {asset.name} as {short_name}...", end=" ")

            dest = build_dir.joinpath(short_name)
            try:
                os.remove(dest)
            except OSError:
                pass
            urllib.request.urlretrieve(asset.browser_download_url, dest)

            print("done.")

    except RateLimitExceededException:
        exit_with_error(
            "Rate limit exceeded.",
            "Set GITHUB_TOKEN environment variable with a valid token to avoid this.",
        )


if __name__ == "__main__":
    download_firmware()
