#!/usr/bin/env python3
#
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Nicolas Schodet
#

"""Convert a font to format used by pbio."""

import argparse
import freetype
from dataclasses import dataclass
from pathlib import PurePath
import re

# Fixed point 26.6 format.
Q = 6

template = """\
// This is a generated file, edit with care.
//
// Original font license:
//
// SPDX-License-Identifier: {self.license}{copyright}
//
// {self.name}, {self.size}px

#include <pbio/font.h>

#if PBIO_CONFIG_IMAGE

static const uint8_t {ident}_data[] = {{
    {data_table},
}};

static const pbio_font_kerning_t {ident}_kernings[] = {{
    // previous char, kerning
{kernings_table}
}};

static const pbio_font_glyph_t {ident}_glyphs[] = {{
    // width, height, advance, left, top, data_index, kerning_index
{glyphs_table}
    {{ 0, 0, 0, 0, 0, 0, {last_kerning_index} }}
}};

const pbio_font_t {ident} = {{
    .first = {self.first},
    .last = {self.last},
    .line_height = {self.line_height},
    .top_max = {self.top_max},
    .glyphs = {ident}_glyphs,
    .data = {ident}_data,
    .kernings = {ident}_kernings,
}};

#endif // PBIO_CONFIG_IMAGE"""


@dataclass
class Glyph:
    """Information on a single glyph."""

    char: int
    width: int
    height: int
    advance: int
    left: int
    top: int
    data: [int]
    kernings: dict[int, int]


class Font:
    """Information on a font."""

    def __init__(
        self,
        name: str,
        ident: str,
        face: freetype.Face,
        size: int,
        first: int,
        last: int,
        license: str,
        copyright: [str],
    ):
        """Convert a face to pbio font format."""
        self.name = name
        self.ident = ident
        self.size = size
        self.first = first
        self.last = last
        self.license = license
        self.copyright = copyright
        self.glyphs = []

        face.set_pixel_sizes(options.size, options.size)

        load_flags = (
            freetype.FT_LOAD_RENDER
            | freetype.FT_LOAD_TARGET_MONO
            | freetype.FT_LOAD_MONOCHROME
        )

        self.line_height = face.size.height >> Q
        self.top_max = 0

        for c in range(first, last + 1):
            face.load_char(c, load_flags)
            bitmap = face.glyph.bitmap
            kernings = {}
            if face.has_kerning:
                for prev in range(options.first, options.last + 1):
                    kerning = face.get_kerning(prev, c).x >> Q
                    if kerning != 0:
                        kernings[prev] = kerning
            glyph = Glyph(
                c,
                bitmap.width,
                bitmap.rows,
                face.glyph.advance.x >> Q,
                face.glyph.bitmap_left,
                face.glyph.bitmap_top,
                self.read_bitmap(bitmap),
                kernings,
            )
            self.glyphs.append(glyph)
            self.top_max = max(self.top_max, glyph.top)

    @staticmethod
    def read_bitmap(bitmap: freetype.Bitmap) -> [int]:
        """Read a freetype bitmap."""
        data = []
        assert bitmap.pixel_mode == freetype.FT_PIXEL_MODE_MONO
        for row in range(0, bitmap.rows):
            index = row * bitmap.pitch
            data += bitmap.buffer[index : index + (bitmap.width + 7) // 8]
        return data

    def dump(self) -> str:
        """Return the font in source code format."""
        data = []
        data_indices = []
        for g in self.glyphs:
            data_indices.append(len(data))
            data.extend(g.data)
        data_table = ",\n    ".join(
            ", ".join(f"{x:#04x}" for x in data[i : i + 12])
            for i in range(0, len(data), 12)
        )
        kernings = []
        kerning_indices = []
        for g in self.glyphs:
            kerning_indices.append(len(kernings))
            for prev, kerning in g.kernings.items():
                cc = chr(prev) + chr(g.char)
                kernings.append(f"    {{ {prev}, {kerning} }}, // {cc!r}")
        kernings_table = "\n".join(kernings)
        last_kerning_index = len(kernings)
        glyphs = []
        for g, di, ki in zip(self.glyphs, data_indices, kerning_indices):
            c = chr(g.char)
            glyphs.append(
                f"    {{ {g.width:2}, {g.height:2}, {g.advance:2}, {g.left:2},"
                f" {g.top:2}, {di:3}, {ki:3} }}, // {c!r}"
            )
        glyphs_table = "\n".join(glyphs)
        ident = "pbio_font_" + self.ident
        copyright = "\n// ".join([""] + self.copyright)
        return template.format_map(vars())


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("font", help="Font file")
p.add_argument("size", type=int, help="Font pixel size")
p.add_argument("--face-index", type=int, default=0, help="Face index inside font file")
p.add_argument("-f", "--first", type=int, default=32, help="First glyph")
p.add_argument("-l", "--last", type=int, default=127, help="Last glyph")
p.add_argument("--license", required=True, help="SPDX License identifier")
p.add_argument(
    "--copyright", action="append", required=True, help="Font copyright text"
)

options = p.parse_args()

name = PurePath(options.font).stem
ident = "{}_{}".format(
    re.sub(r"[^a-z0-9_]", "", name.lower().replace("-", "_")), options.size
)
face = freetype.Face(options.font, options.face_index)
font = Font(
    name,
    ident,
    face,
    options.size,
    options.first,
    options.last,
    options.license,
    options.copyright,
)
print(font.dump())
