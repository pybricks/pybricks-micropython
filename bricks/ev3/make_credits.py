#!/usr/bin/env python3
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description="Generate EV3 credits.")
parser.add_argument("dest", help="Destination build folder.")
args = parser.parse_args()

build_dir = Path(args.dest)
build_dir.mkdir(parents=True, exist_ok=True)

source_dir = Path(__file__).parent
source_path = (
    source_dir / "ci_credits.txt"
    if (source_dir / "ci_credits.txt").exists()
    else source_dir / "credits.txt"
)
names = open(source_path, "r").read().splitlines()

# Write the result as a list of C strings.
destination = open(build_dir / "hmi_lcd_ui_credits.c", "w")
destination.write("""// SPDX-License-Identifier: MIT
//Copyright (c) 2026 The Pybricks Authors

#include <stddef.h>

const char * const hmi_lcd_ui_credits_names[] = {
""")

for name in names:
    destination.write(f'    "{name}",\n')

destination.write(
    "};\n\nconst size_t hmi_lcd_ui_credits_size = sizeof(hmi_lcd_ui_credits_names) / sizeof(hmi_lcd_ui_credits_names[0]);"
)
