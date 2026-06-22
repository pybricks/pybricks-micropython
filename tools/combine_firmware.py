#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

"""
Combines several single-variant Pybricks firmware.zip files into one multi-variant
firmware.zip.

Each input zip is produced by a normal brick build and contains exactly one entry
in its metadata ``variants`` list (see tools/metadata.py). This tool merges them
into a single zip that ships all architecture variants of the same product, so the
flasher can pick the right one for the connected hub.

The combined zip contains:
    firmware-base-<platform>.bin   one per variant (renamed from firmware-base.bin)
    firmware.metadata.json         merged metadata with all variants
    ReadMe_OSS.txt                 single copy (asserted identical across variants)

The shared top-level metadata fields (metadata-version, firmware-version,
device-id) must be identical across all inputs.
"""

import argparse
import json
import sys
import zipfile

# Top-level metadata fields that must match across all variants.
SHARED_FIELDS = ("metadata-version", "firmware-version", "device-id")

BASE_BIN = "firmware-base.bin"
METADATA_NAME = "firmware.metadata.json"
README_NAME = "ReadMe_OSS.txt"


def combine(out_path: str, in_paths: list):
    shared = None
    readme = None
    variants = []
    # Maps output bin name -> raw firmware base binary.
    firmware_blobs = {}

    for in_path in in_paths:
        with zipfile.ZipFile(in_path) as archive:
            metadata = json.loads(archive.read(METADATA_NAME))

            current_shared = {key: metadata[key] for key in SHARED_FIELDS}
            if shared is None:
                shared = current_shared
            elif current_shared != shared:
                print(
                    f"Shared metadata mismatch in {in_path}: "
                    f"{current_shared} != {shared}",
                    file=sys.stderr,
                )
                exit(1)

            current_readme = archive.read(README_NAME)
            if readme is None:
                readme = current_readme
            elif current_readme != readme:
                print(f"{README_NAME} differs in {in_path}", file=sys.stderr)
                exit(1)

            if len(metadata["variants"]) != 1:
                print(
                    f"Expected exactly one variant in {in_path}, "
                    f"got {len(metadata['variants'])}",
                    file=sys.stderr,
                )
                exit(1)

            variant = metadata["variants"][0]
            platform = variant["platform"]
            bin_name = f"firmware-base-{platform}.bin"

            if bin_name in firmware_blobs:
                print(f"Duplicate platform {platform}", file=sys.stderr)
                exit(1)

            variant["firmware"] = bin_name
            variants.append(variant)
            firmware_blobs[bin_name] = archive.read(BASE_BIN)

    combined_metadata = dict(shared)
    combined_metadata["variants"] = variants

    with zipfile.ZipFile(out_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for bin_name, blob in firmware_blobs.items():
            archive.writestr(bin_name, blob)
        archive.writestr(
            METADATA_NAME, json.dumps(combined_metadata, indent=4, sort_keys=True)
        )
        archive.writestr(README_NAME, readme)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Combine single-variant firmware zips into one multi-variant zip."
    )
    parser.add_argument(
        "out_file",
        metavar="<output-zip>",
        help="combined firmware zip to create",
    )
    parser.add_argument(
        "in_files",
        metavar="<variant-zip>",
        nargs="+",
        help="single-variant firmware zip(s) to combine",
    )

    args = parser.parse_args()
    combine(args.out_file, args.in_files)
