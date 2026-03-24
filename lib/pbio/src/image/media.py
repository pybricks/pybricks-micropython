#!/usr/bin/env python3
import argparse
import sys
import io
from pathlib import Path
from PIL import Image
import cairosvg

parser = argparse.ArgumentParser(description="Convert image files to C structs.")
parser.add_argument("-o", "--output", type=Path, help="Output file name for C structs.")
parser.add_argument(
    "--decls", type=Path, help="Output file name for struct declarations."
)
parser.add_argument(
    "--decls-include",
    help="Struct declaration include file name, generated from output file if given.",
)
parser.add_argument("--attrs", type=Path, help="Output file name for attribute list.")
parser.add_argument("images", type=Path, nargs="+", help="Input file name(s).")
args = parser.parse_args()

if args.attrs:
    if args.decls_include is not None:
        decls_include = args.decls_include
    elif args.decls is not None:
        decls_include = args.decls.name
    else:
        parser.error("Need struct declaration include file name.")


def load_image(path):
    if path.suffix == ".svg":
        with open(path, "rb") as f:
            png_bytes = cairosvg.svg2png(file_obj=f)
        return Image.open(io.BytesIO(png_bytes))
    else:
        return Image.open(path)


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
for img_path in args.images:
    name = img_path.stem
    if args.output:
        with load_image(img_path) as img:
            width, height, bin_data = image_to_8bit_map(img)
            results[name] = (width, height, bin_data)
    else:
        results[name] = (0, 0, None)


externs = ""
structs = ""
qstrtab = ""

for name in sorted(results):
    width, height, bin_data = results[name]

    if args.output:
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


HEADER = """// Generated file, edit with care.

#include <pbio/image.h>
"""

if args.output:
    with open(args.output, "w") as f:
        f.write(HEADER)
        f.write(structs)

if args.decls:
    guard = args.decls.name.upper().replace(".", "_")
    with open(args.decls, "w") as f:
        f.write(HEADER)
        f.write(f"#ifndef _{guard}_\n")
        f.write(f"#define _{guard}_\n\n")
        f.write(externs)
        f.write(f"#endif // _{guard}_\n")

if args.attrs:
    with open(args.attrs, "w") as f:
        f.write(HEADER)
        f.write(f'#include "{decls_include}"\n\n')
        f.write("#include <py/obj.h>\n\n")
        f.write(
            "static const mp_rom_map_elem_t pb_type_image_attributes_dict_table[] = {\n"
        )
        f.write(qstrtab)
        f.write("};\n")
        f.write(
            "MP_DEFINE_CONST_DICT(pb_type_image_attributes_dict, pb_type_image_attributes_dict_table);"
        )
