// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Nicolas Schodet

/**
 * @addtogroup Font Text font.
 * @{
 */

#ifndef _PBIO_FONT_H_
#define _PBIO_FONT_H_

#include <pbio/config.h>

#include <stdint.h>

/**
 * Single glyph.
 */
typedef struct _pbio_font_glyph_t {
    /**
     * Width of glyph bitmap.
     */
    uint8_t width;
    /**
     * Height of glyph bitmap.
     */
    uint8_t height;
    /**
     * Advance to next character in horizontal direction.
     */
    uint8_t advance;
    /**
     * Horizontal distance between pen position and leftmost bitmap column.
     * Positive values goes left.
     */
    int8_t left;
    /**
     * Vertical distance between baseline and topmost bitmap row. Positive
     * values goes up.
     */
    int8_t top;
    /**
     * Index of bitmap start in data table.
     */
    uint16_t data_index;
    /**
     * Start index of kerning values in kerning table. The Stop index is the
     * start index of the next glyph.
     */
    uint16_t kerning_index;
} pbio_font_glyph_t;

/**
 * Kerning information for a glyph.
 */
typedef struct _pbio_font_kerning_t {
    /**
     * Previous glyph.
     */
    uint8_t previous;
    /**
     * Kerning from previous glyph to current one.
     */
    int8_t kerning;
} pbio_font_kerning_t;

/**
 * Font.
 *
 * Every bitmap data is composed of one or several rows as indicated in the
 * glyph structure, with the top row first. Eight pixels are packed into
 * a bytes, with the first leftmost pixel in the most significant bit. The
 * bitmap width is padded to the next multiple of eight.
 */
typedef struct _pbio_font_t {
    /**
     * First glyph.
     */
    uint8_t first;
    /**
     * Last glyph.
     */
    uint8_t last;
    /**
     * Line height, number of pixels between consecutive baselines.
     */
    uint8_t line_height;
    /**
     * Maximum value of glyph top.
     */
    int8_t top_max;
    /**
     * Glyph table. Must contain a dummy last glyph for the last kerning stop
     * index.
     */
    const pbio_font_glyph_t *glyphs;
    /**
     * Data table with bitmaps.
     */
    const uint8_t *data;
    /**
     * Kerning table, may be NULL if no kerning.
     */
    const pbio_font_kerning_t *kernings;
} pbio_font_t;

#if PBIO_CONFIG_IMAGE

extern const pbio_font_t pbio_font_liberationsans_regular_14;
extern const pbio_font_t pbio_font_terminus_normal_16;
extern const pbio_font_t pbio_font_mono_8x5_8;

#endif // PBIO_CONFIG_IMAGE

#endif // _PBIO_FONT_H_

/** @} */
