#!/usr/bin/env python3
import argparse
from pathlib import Path
from PIL import Image
import cairosvg

# Take build directory as argument to save generated C files and PNG files.
parser = argparse.ArgumentParser(description="Convert SVG files to PNG.")
parser.add_argument("dest", help="Destination build folder for PNG files.")
args = parser.parse_args()

build_dir = Path(args.dest)
build_dir.mkdir(parents=True, exist_ok=True)
media_dir = Path(__file__).parent / "media"

# Convert all SVG files in media_dir to PNG and save in build_dir if not already present.
svg_files = media_dir.rglob("*.svg")
for svg in svg_files:
    png = svg.with_suffix(".png").name
    png_path = build_dir / png
    if png_path.exists():
        continue
    with open(svg, "rb") as svg_file:
        png_bytes = cairosvg.svg2png(file_obj=svg_file)
        with open(png_path, "wb") as out_png:
            out_png.write(png_bytes)

# Collect all image files in media_dir (png, bmp, jpg) and build_dir (png), including subfolders.
media_images = (
    list(media_dir.rglob("*.png"))
    + list(media_dir.rglob("*.bmp"))
    + list(media_dir.rglob("*.jpg"))
    + list(build_dir.rglob("*.png"))
)


# Convert rgba to monochrome, treating fully transparent pixels as white.
def is_black(r, g, b, a):
    if a == 0:
        return 0
    return 1 if (r + g + b) < (128 * 3) else 0


def image_to_8bit_map(img):
    img = img.convert("RGBA")
    width, height = img.size
    pixels = img.load()
    mono = [is_black(*pixels[x, y]) for y in range(height) for x in range(width)]

    # go in chunks of 8 pixels and pack into a byte
    data = []
    for i in range(0, len(mono), 8):
        byte = 0
        for j in range(8):
            if i + j < len(mono):
                byte |= mono[i + j] << (7 - j)
        data.append(byte)

    return width, height, bytes(data)


# Process each image.
results = {}
for img_path in media_images:
    with Image.open(img_path) as img:
        name = Path(img_path.name).stem
        width, height, bin_data = image_to_8bit_map(img)
        results[name] = (width, height, bin_data)


externs = ""
structs = ""
qstrtab = ""

for name in sorted(results):
    width, height, bin_data = results[name]

    # Parse bytes for printing.
    bytes_per_line = 12
    lines = []
    for i in range(0, len(bin_data), bytes_per_line):
        chunk = bin_data[i : i + bytes_per_line]
        line = "    " + ", ".join(f"0x{val:02x}" for val in chunk)
        lines.append(line)
    data_literal = ",\n".join(lines) + ","

    # Printed C structs.
    structs += f"static const uint8_t {name}_data[] = {{\n{data_literal}\n}};\n\n"
    structs += (
        f"const pbio_image_monochrome_t pbio_image_media_{name} = {{\n"
        f"    .width = {width},\n"
        f"    .height = {height},\n"
        f"    .data = {name}_data,\n"
        f"}};\n"
    )

    # Printed header and QSTR table entries.
    externs += f"extern const pbio_image_monochrome_t pbio_image_media_{name};\n\n"
    qstrtab += f"    {{ MP_ROM_QSTR(MP_QSTR_{name.upper()}), MP_ROM_PTR(&pbio_image_media_{name}) }},\n"


HEADER = """// SPDX-License-Identifier: MIT
//Copyright (c) 2025 The Pybricks Authors

#include <pbio/image.h>
"""

with open(build_dir / "pbio_image_media.c", "w") as f:
    f.write(HEADER)
    f.write('#include "pbio_image_media.h"\n\n')
    f.write(structs)

with open(build_dir / "pbio_image_media.h", "w") as f:
    f.write(HEADER)
    f.write("#ifndef _PBIO_IMAGE_MEDIA_H_\n")
    f.write("#define _PBIO_IMAGE_MEDIA_H_\n\n")
    f.write(externs)
    f.write("#endif // _PBIO_IMAGE_MEDIA_H_\n")

with open(build_dir / "pb_type_image_attributes.c", "w") as f:
    f.write(HEADER)
    f.write('#include "pbio_image_media.h"\n\n')
    f.write("#include <py/obj.h>\n\n")
    f.write(
        "static const mp_rom_map_elem_t pb_type_image_attributes_dict_table[] = {\n"
    )
    f.write(qstrtab)
    f.write("};\n")
    f.write(
        "MP_DEFINE_CONST_DICT(pb_type_image_attributes_dict, pb_type_image_attributes_dict_table);"
    )
