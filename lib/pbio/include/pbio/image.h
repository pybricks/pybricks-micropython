// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Nicolas Schodet

/**
 * @addtogroup Image Grayscale image manipulation and drawing.
 * @{
 */

#ifndef _PBIO_IMAGE_H_
#define _PBIO_IMAGE_H_

#include <pbio/config.h>

#include <stdint.h>

/**
 * Image container.
 *
 * Images are oriented with the 0,0 coordinate corresponding to the top left
 * corner. But this is only an arbitrary convention to write comments.
 */
typedef struct _pbio_image_t {
    /**
     * Start of pixel buffer storing the pixels values.
     *
     * Each pixel is stored using one byte and is expected to be a value
     * between 0 and some number fitting in one byte. This code has no opinion
     * on the maximum value as long as it fits.
     *
     * Rows are continuous in memory.
     *
     * This is not an owning type, buffer lifetime must be handled externally.
     * It can actually point to memory shared with another image.
     */
    uint8_t *pixels;
    /**
     * Width of the image, or number of columns.
     */
    int width;
    /**
     * Height of the image, or number of rows.
     */
    int height;
    /**
     * Distance in bytes inside the pixel buffer to go from one row to the
     * next one.
     *
     * This is signed on purpose so that an image can be mirrored cheaply
     * using negative value.
     */
    int stride;
} pbio_image_t;

#if PBIO_CONFIG_IMAGE

void pbio_image_init(pbio_image_t *image, uint8_t *pixels, int width,
    int height, int stride);

void pbio_image_init_sub(pbio_image_t *image, const pbio_image_t *source,
    int x, int y, int width, int height);

void pbio_image_fill(pbio_image_t *image, uint8_t value);

void pbio_image_draw_image(pbio_image_t *image, const pbio_image_t *source,
    int x, int y);

void pbio_image_draw_image_transparent(pbio_image_t *image,
    const pbio_image_t *source, int x, int y, uint8_t value);

void pbio_image_draw_pixel(pbio_image_t *image, int x, int y, uint8_t value);

void pbio_image_draw_hline(pbio_image_t *image, int x, int y, int l,
    uint8_t value);

void pbio_image_draw_vline(pbio_image_t *image, int x, int y, int l,
    uint8_t value);

void pbio_image_draw_line(pbio_image_t *image, int x1, int y1, int x2, int y2,
    uint8_t value);

void pbio_image_draw_thick_line(pbio_image_t *image, int x1, int y1, int x2,
    int y2, int thickness, uint8_t value);

void pbio_image_draw_rect(pbio_image_t *image, int x, int y, int width,
    int height, uint8_t value);

void pbio_image_fill_rect(pbio_image_t *image, int x, int y, int width,
    int height, uint8_t value);

void pbio_image_draw_rounded_rect(pbio_image_t *image, int x, int y,
    int width, int height, int r, uint8_t value);

void pbio_image_fill_rounded_rect(pbio_image_t *image, int x, int y,
    int width, int height, int r, uint8_t value);

void pbio_image_draw_circle(pbio_image_t *image, int x, int y, int r,
    uint8_t value);

void pbio_image_fill_circle(pbio_image_t *image, int x, int y, int r,
    uint8_t value);

#endif // PBIO_CONFIG_IMAGE

#endif // _PBIO_IMAGE_H_

/** @} */
