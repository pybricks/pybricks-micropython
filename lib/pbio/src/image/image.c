// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Nicolas Schodet

#include <pbio/config.h>

#if PBIO_CONFIG_IMAGE

#include <pbio/image.h>

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>

/**
 * Clip range between 0 and given length, return if out.
 * @param [in,out] c1  First coordinate, included.
 * @param [in,out] c2  Second coordinate, excluded.
 * @param [in]     l   Length.
 *
 * First coordinate must not be greater than the second one.
 */
#define clip_or_return(c1, c2, l) \
    do { \
        if ((c1) >= (l) || (c2) <= 0) { \
            return; \
        } \
        if ((c1) < 0) { \
            (c1) = 0; \
        } \
        if ((c2) > (l)) { \
            (c2) = (l); \
        } \
    } while (0)

/**
 * Initialize an image, using external storage.
 * @param [out] image   Uninitialized image to initialize.
 * @param [in]  pixels  Buffer storing the pixels values, not changed.
 * @param [in]  width   Number of columns.
 * @param [in]  height  Number of rows.
 * @param [in]  stride  Distance in bytes inside the pixel buffer to go from
 *                      one row to the next one.
 *
 * No memory allocation is done. This function takes an uninitialized image
 * structure. The pixel buffer is left untouched, pbio_image_fill() can be
 * used to clear it.
 */
void pbio_image_init(pbio_image_t *image, uint8_t *pixels, int width,
    int height, int stride) {
    image->pixels = pixels;
    image->width = width;
    image->height = height;
    image->stride = stride;
    image->print_font = NULL;
    image->print_x_left = 0;
    image->print_y_top = 0;
    image->print_value = 0;
}

/**
 * Initialize an image as a viewport into another bigger image.
 * @param [out] image   Uninitialized image to initialize.
 * @param [in]  source  Source image.
 * @param [in]  x       X coordinate of the top-left point of the viewport in
 *                      source.
 * @param [in]  y       Y coordinate of the top-left point of the viewport in
 *                      source.
 * @param [in]  width   Width of the viewport.
 * @param [in]  height  Height of the viewport.
 *
 * After this call, the two images shares the same buffer, modifying a pixel
 * inside the viewport in one image will change the same pixel in the other
 * image.
 *
 * Clipping: viewport is clipped to source image dimensions. If completely out
 * of the source image, the destination image may be empty.
 */
void pbio_image_init_sub(pbio_image_t *image, const pbio_image_t *source,
    int x, int y, int width, int height) {
    // Start with an empty image in case of early return.
    pbio_image_init(image, source->pixels, 0, 0, 0);

    // Eliminate weird cases.
    if (width <= 0 || height <= 0) {
        return;
    }

    // Clipping, may result in an empty image.
    int x2 = x + width;
    int y2 = y + height;
    clip_or_return(x, x2, source->width);
    clip_or_return(y, y2, source->height);

    // Select the right part of source image.
    pbio_image_init(image, source->pixels + y * source->stride + x, x2 - x,
        y2 - y, source->stride);

    // Reuse the same font and value.
    image->print_font = source->print_font;
    image->print_value = source->print_value;
}

/**
 * Fill an image with a value.
 * @param [in] image  Image to fill.
 * @param [in] value  Pixel value.
 *
 * This also resets the text printing position.
 */
void pbio_image_fill(pbio_image_t *image, uint8_t value) {
    uint8_t *p = image->pixels;
    for (int h = image->height; h; h--) {
        memset(p, value, image->width);
        p += image->stride;
    }
    image->print_x_left = 0;
    image->print_y_top = 0;
}

/**
 * Draw an image inside another image.
 * @param [in] image   Destination image to draw into.
 * @param [in] source  Source image.
 * @param [in] x       X coordinate of the top-left point in destination
 *                     image.
 * @param [in] y       Y coordinate of the top-left point in destination
 *                     image.
 *
 * Source image pixels are copied into destination image.
 *
 * Clipping: drawing is clipped to destination image dimensions.
 */
void pbio_image_draw_image(pbio_image_t *image, const pbio_image_t *source,
    int x, int y) {
    // Clipping.
    int ox = x;
    int oy = y;
    int x2 = x + source->width;
    int y2 = y + source->height;
    clip_or_return(x, x2, image->width);
    clip_or_return(y, y2, image->height);

    // Copy pixels.
    uint8_t *src = source->pixels + (y - oy) * source->stride + (x - ox);
    uint8_t *dst = image->pixels + y * image->stride + x;
    int w = x2 - x;
    for (int h = y2 - y; h; h--) {
        memcpy(dst, src, w);
        dst += image->stride;
        src += source->stride;
    }
}

/**
 * Draw an image inside another image with transparency.
 * @param [in] image   Destination image to draw into.
 * @param [in] source  Source image.
 * @param [in] x       X coordinate of the top-left point in destination
 *                     image.
 * @param [in] y       Y coordinate of the top-left point in destination
 *                     image.
 * @param [in] value   Pixel value in source image considered transparent.
 *
 * Source image pixels are copied into destination image. When a source pixel
 * matches the transparent value, the corresponding destination pixel is left
 * untouched.
 *
 * Clipping: drawing is clipped to destination image dimensions.
 */
void pbio_image_draw_image_transparent(pbio_image_t *image,
    const pbio_image_t *source, int x, int y, uint8_t value) {
    // Clipping.
    int ox = x;
    int oy = y;
    int x2 = x + source->width;
    int y2 = y + source->height;
    clip_or_return(x, x2, image->width);
    clip_or_return(y, y2, image->height);

    // Draw pixels.
    uint8_t *src = source->pixels + (y - oy) * source->stride + (x - ox);
    uint8_t *dst = image->pixels + y * image->stride + x;
    int w = x2 - x;
    for (int h = y2 - y; h; h--) {
        for (int i = w; i; i--) {
            uint8_t c = *src;
            if (c != value) {
                *dst = c;
            }
            src++;
            dst++;
        }
        dst += image->stride - w;
        src += source->stride - w;
    }
}

/**
 * Draw a single pixel.
 * @param [in] image  Image to draw into.
 * @param [in] x      X coordinate of the pixel.
 * @param [in] y      Y coordinate of the pixel.
 * @param [in] value  New pixel value.
 *
 * Clipping: pixel is not drawn if coordinate is outside of the image.
 */
void pbio_image_draw_pixel(pbio_image_t *image, int x, int y, uint8_t value) {
    // Clipping.
    if (x < 0 || x >= image->width || y < 0 || y >= image->height) {
        return;
    }

    // Draw pixel.
    uint8_t *p = image->pixels + y * image->stride + x;
    *p = value;
}

/**
 * Draw a horizontal line.
 * @param [in] image  Image to draw into.
 * @param [in] x      X coordinate of the leftmost pixel.
 * @param [in] y      Y coordinate of the line.
 * @param [in] l      Length of the line.
 * @param [in] value  Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_hline(pbio_image_t *image, int x, int y, int l,
    uint8_t value) {
    // Eliminate weird cases.
    if (l <= 0) {
        return;
    }

    // Clipping.
    int x2 = x + l;
    clip_or_return(x, x2, image->width);
    if (y < 0 || y >= image->height) {
        return;
    }

    // Draw line.
    uint8_t *p = image->pixels + y * image->stride + x;
    memset(p, value, x2 - x);
}

/**
 * Draw a vertical line.
 * @param [in] image  Image to draw into.
 * @param [in] x      X coordinate of the line.
 * @param [in] y      Y coordinate of the topmost pixel.
 * @param [in] l      Length of the line.
 * @param [in] value  Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_vline(pbio_image_t *image, int x, int y, int l,
    uint8_t value) {
    // Eliminate weird cases.
    if (l <= 0) {
        return;
    }

    // Clipping.
    int y2 = y + l;
    if (x < 0 || x >= image->width) {
        return;
    }
    clip_or_return(y, y2, image->height);

    // Draw line.
    uint8_t *p = image->pixels + y * image->stride + x;
    for (int h = y2 - y; h; h--) {
        *p = value;
        p += image->stride;
    }
}

/**
 * Draw a line with a flat slope (less or equal to 1).
 * @param [in] image  Image to draw into.
 * @param [in] x1     X coordinate of the first end.
 * @param [in] y1     Y coordinate of the first end.
 * @param [in] x2     X coordinate of the second end.
 * @param [in] y2     Y coordinate of the second end.
 * @param [in] value  Pixel value.
 *
 * This is an internal function, x2 must be greater or equal to x1.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
static void pbio_image_draw_line_flat(pbio_image_t *image, int x1, int y1,
    int x2, int y2, uint8_t value) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int ydir = 1;
    int x, y, err;

    // Fall back to horizontal line, much faster.
    if (dy == 0) {
        pbio_image_draw_hline(image, x1, y1, dx + 1, value);
        return;
    }

    // Clipping, X out of image.
    if (x1 >= image->width || x2 < 0) {
        return;
    }

    // Check Y direction.
    if (dy < 0) {
        dy = -dy;
        ydir = -1;
    }

    // Error is scaled by 2 * dx, offset with one half to look at mid point.
    err = -dx;
    y = y1;

    // Skip pixels left of image.
    if (x1 < 0) {
        err += -x1 * dy * 2;
        int yskip = (err + dx * 2) / (dx * 2);
        err -= yskip * dx * 2;
        y += yskip * ydir;
        x1 = 0;
    }

    // Skip pixels right of image.
    if (x2 >= image->width) {
        x2 = image->width - 1;
    }

    // Draw.
    x = x1;
    do {
        pbio_image_draw_pixel(image, x, y, value);
        err += dy * 2;
        if (err >= 0) {
            err -= dx * 2;
            y += ydir;
        }
        x++;
    } while (x <= x2);
}

/**
 * Draw a line with a steep slope (more than 1).
 * @param [in] image  Image to draw into.
 * @param [in] x1     X coordinate of the first end.
 * @param [in] y1     Y coordinate of the first end.
 * @param [in] x2     X coordinate of the second end.
 * @param [in] y2     Y coordinate of the second end.
 * @param [in] value  Pixel value.
 *
 * This is an internal function, y2 must be greater or equal to y1.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
static void pbio_image_draw_line_steep(pbio_image_t *image, int x1, int y1,
    int x2, int y2, uint8_t value) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int xdir = 1;
    int x, y, err;

    // Fall back to vertical line, much faster.
    if (dx == 0) {
        pbio_image_draw_vline(image, x1, y1, dy + 1, value);
        return;
    }

    // Clipping, Y out of image.
    if (y1 >= image->height || y2 < 0) {
        return;
    }

    // Check X direction.
    if (dx < 0) {
        dx = -dx;
        xdir = -1;
    }

    // Error is scaled by 2 * dy, offset with one half to look at mid point.
    err = -dy;
    x = x1;

    // Skip pixels above image.
    if (y1 < 0) {
        err += -y1 * dx * 2;
        int xskip = (err + dy * 2) / (dy * 2);
        err -= xskip * dy * 2;
        x += xskip * xdir;
        y1 = 0;
    }

    // Skip pixels bellow image.
    if (y2 >= image->height) {
        y2 = image->height - 1;
    }

    // Draw.
    y = y1;
    do {
        pbio_image_draw_pixel(image, x, y, value);
        err += dx * 2;
        if (err >= 0) {
            err -= dy * 2;
            x += xdir;
        }
        y++;
    } while (y <= y2);
}

/**
 * Draw a line.
 * @param [in] image  Image to draw into.
 * @param [in] x1     X coordinate of the first end.
 * @param [in] y1     Y coordinate of the first end.
 * @param [in] x2     X coordinate of the second end.
 * @param [in] y2     Y coordinate of the second end.
 * @param [in] value  Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_line(pbio_image_t *image, int x1, int y1, int x2, int y2,
    uint8_t value) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;

    if (abs_dx >= abs_dy) {
        // Flat slope, X always increasing.
        if (dx > 0) {
            pbio_image_draw_line_flat(image, x1, y1, x2, y2, value);
        } else {
            pbio_image_draw_line_flat(image, x2, y2, x1, y1, value);
        }
    } else {
        // Steep slope, Y always increasing.
        if (dy > 0) {
            pbio_image_draw_line_steep(image, x1, y1, x2, y2, value);
        } else {
            pbio_image_draw_line_steep(image, x2, y2, x1, y1, value);
        }
    }
}

/**
 * Draw a thick line.
 * @param [in] image      Image to draw into.
 * @param [in] x1         X coordinate of the first end.
 * @param [in] y1         Y coordinate of the first end.
 * @param [in] x2         X coordinate of the second end.
 * @param [in] y2         Y coordinate of the second end.
 * @param [in] thickness  Line thickness.
 * @param [in] value      Pixel value.
 *
 * When line thickness is odd, pixels are centered on the line. When even,
 * line is thicker on left side, when looking from first end to second end.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_thick_line(pbio_image_t *image, int x1, int y1, int x2,
    int y2, int thickness, uint8_t value) {
    // Eliminate weird cases.
    if (thickness <= 0) {
        return;
    }

    // Fall back to regular line.
    if (thickness <= 1) {
        pbio_image_draw_line(image, x1, y1, x2, y2, value);
        return;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    int i;
    int offset = thickness / 2;

    if (abs_dx >= abs_dy) {
        // Flat slope, X always increasing.
        if (dx > 0) {
            for (i = 0; i < thickness; i++) {
                pbio_image_draw_line_flat(image, x1, y1 + i - offset,
                    x2, y2 + i - offset, value);
            }
        } else {
            for (i = 0; i < thickness; i++) {
                pbio_image_draw_line_flat(image, x2, y2 - i + offset,
                    x1, y1 - i + offset, value);
            }
        }
    } else {
        // Steep slope, Y always increasing.
        if (dy > 0) {
            for (i = 0; i < thickness; i++) {
                pbio_image_draw_line_steep(image, x1 - i + offset, y1,
                    x2 - i + offset, y2, value);
            }
        } else {
            for (i = 0; i < thickness; i++) {
                pbio_image_draw_line_steep(image, x2 + i - offset, y2,
                    x1 + i - offset, y1, value);
            }
        }
    }
}

/**
 * Draw a rectangle.
 * @param [in] image   Image to draw into.
 * @param [in] x       X coordinate of the top-left corner.
 * @param [in] y       Y coordinate of the top-left corner.
 * @param [in] width   Rectangle width.
 * @param [in] height  Rectangle height.
 * @param [in] value   Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_rect(pbio_image_t *image, int x, int y, int width,
    int height, uint8_t value) {
    // Draw.
    pbio_image_draw_hline(image, x, y, width, value);
    if (height > 1) {
        pbio_image_draw_hline(image, x, y + height - 1, width, value);
    }
    pbio_image_draw_vline(image, x, y + 1, height - 2, value);
    if (width > 1) {
        pbio_image_draw_vline(image, x + width - 1, y + 1, height - 2, value);
    }
}

/**
 * Draw a filled rectangle.
 * @param [in] image   Image to draw into.
 * @param [in] x       X coordinate of the top-left corner.
 * @param [in] y       Y coordinate of the top-left corner.
 * @param [in] width   Rectangle width.
 * @param [in] height  Rectangle height.
 * @param [in] value   Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_fill_rect(pbio_image_t *image, int x, int y, int width,
    int height, uint8_t value) {
    // Eliminate weird cases.
    if (width <= 0 || height <= 0) {
        return;
    }

    // Clipping.
    int x2 = x + width;
    int y2 = y + height;
    clip_or_return(x, x2, image->width);
    clip_or_return(y, y2, image->height);

    // Draw.
    uint8_t *p = image->pixels + y * image->stride + x;
    for (int h = y2 - y; h; h--) {
        memset(p, value, x2 - x);
        p += image->stride;
    }
}

/**
 * Draw a rounded rectangle.
 * @param [in] image   Image to draw into.
 * @param [in] x       X coordinate of the top-left corner.
 * @param [in] y       Y coordinate of the top-left corner.
 * @param [in] width   Rectangle width.
 * @param [in] height  Rectangle height.
 * @param [in] r       Corner radius.
 * @param [in] value   Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_rounded_rect(pbio_image_t *image, int x, int y,
    int width, int height, int r, uint8_t value) {
    // Eliminate weird cases.
    if (width <= 0 || height <= 0) {
        return;
    }
    if (r > (width - 1) / 2) {
        r = (width - 1) / 2;
    }
    if (r > (height - 1) / 2) {
        r = (height - 1) / 2;
    }

    // Fall back to regular rectangle.
    if (r < 1) {
        pbio_image_draw_rect(image, x, y, width, height, value);
        return;
    }

    // Draw corners.
    int dx = 0, dy = r;
    int r2 = r * r;
    int xr1 = x + r;
    int yr1 = y + r;
    int xr2 = x + width - 1 - r;
    int yr2 = y + height - 1 - r;
    while (dx < dy) {
        dx++;
        if (dx * dx + dy * dy > r2) {
            dy--;
        }
        pbio_image_draw_pixel(image, xr1 - dx, yr1 - dy, value);
        pbio_image_draw_pixel(image, xr1 - dy, yr1 - dx, value);
        pbio_image_draw_pixel(image, xr2 + dx, yr1 - dy, value);
        pbio_image_draw_pixel(image, xr2 + dy, yr1 - dx, value);
        pbio_image_draw_pixel(image, xr1 - dx, yr2 + dy, value);
        pbio_image_draw_pixel(image, xr1 - dy, yr2 + dx, value);
        pbio_image_draw_pixel(image, xr2 + dx, yr2 + dy, value);
        pbio_image_draw_pixel(image, xr2 + dy, yr2 + dx, value);
    }

    // Draw sides.
    int wr = width - 2 * r;
    int hr = height - 2 * r;
    pbio_image_draw_hline(image, xr1, y, wr, value);
    pbio_image_draw_hline(image, xr1, y + height - 1, wr, value);
    pbio_image_draw_vline(image, x, yr1, hr, value);
    pbio_image_draw_vline(image, x + width - 1, yr1, hr, value);
}

/**
 * Draw a filled rounded rectangle.
 * @param [in] image   Image to draw into.
 * @param [in] x       X coordinate of the top-left corner.
 * @param [in] y       Y coordinate of the top-left corner.
 * @param [in] width   Rectangle width.
 * @param [in] height  Rectangle height.
 * @param [in] r       Corner radius.
 * @param [in] value   Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_fill_rounded_rect(pbio_image_t *image, int x, int y,
    int width, int height, int r, uint8_t value) {
    // Eliminate weird cases.
    if (width <= 0 || height <= 0) {
        return;
    }
    if (r > (width - 1) / 2) {
        r = (width - 1) / 2;
    }
    if (r > (height - 1) / 2) {
        r = (height - 1) / 2;
    }

    // Fall back to regular rectangle.
    if (r < 1) {
        pbio_image_fill_rect(image, x, y, width, height, value);
        return;
    }

    // Draw top and bottom.
    int dx = 0, dy = r;
    int r2 = r * r;
    int xr1 = x + r;
    int yr1 = y + r;
    int yr2 = y + height - 1 - r;
    int wr = width - 2 * r;
    while (dx <= dy) {
        // Optimization opportunity: when dy is not moving, the same pixels
        // are drawn again and again.
        pbio_image_draw_hline(image, xr1 - dx, yr1 - dy, wr + 2 * dx, value);
        pbio_image_draw_hline(image, xr1 - dx, yr2 + dy, wr + 2 * dx, value);
        if (dx != dy) {
            pbio_image_draw_hline(image, xr1 - dy, yr1 - dx, wr + 2 * dy,
                value);
            pbio_image_draw_hline(image, xr1 - dy, yr2 + dx, wr + 2 * dy,
                value);
        }
        dx++;
        if (dx * dx + dy * dy > r2) {
            dy--;
        }
    }

    // Draw middle section.
    pbio_image_fill_rect(image, x, yr1 + 1, width, height - 2 - 2 * r, value);
}

/**
 * Draw a circle.
 * @param [in] image  Image to draw into.
 * @param [in] x      X coordinate of the circle center.
 * @param [in] y      Y coordinate of the circle center.
 * @param [in] r      Circle radius.
 * @param [in] value  Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_draw_circle(pbio_image_t *image, int x, int y, int r,
    uint8_t value) {
    // Eliminate weird cases.
    if (r <= 0) {
        return;
    }

    // Draw.
    int dx = 0, dy = r;
    int err = 5 - r * 4;
    while (dx <= dy) {
        pbio_image_draw_pixel(image, x + dx, y + dy, value);
        pbio_image_draw_pixel(image, x - dx, y + dy, value);
        pbio_image_draw_pixel(image, x + dx, y - dy, value);
        pbio_image_draw_pixel(image, x - dx, y - dy, value);
        pbio_image_draw_pixel(image, x + dy, y + dx, value);
        pbio_image_draw_pixel(image, x - dy, y + dx, value);
        pbio_image_draw_pixel(image, x + dy, y - dx, value);
        pbio_image_draw_pixel(image, x - dy, y - dx, value);
        if (err > 0) {
            dy--;
            err -= dy * 8;
        }
        dx++;
        err += 4 + dx * 8;
    }
}

/**
 * Draw a disc.
 * @param [in] image  Image to draw into.
 * @param [in] x      X coordinate of the disc center.
 * @param [in] y      Y coordinate of the disc center.
 * @param [in] r      Disc radius.
 * @param [in] value  Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_fill_circle(pbio_image_t *image, int x, int y, int r,
    uint8_t value) {
    // Eliminate weird cases.
    if (r <= 0) {
        return;
    }

    // Draw.
    int dx = 0, dy = r;
    int err = 5 - r * 4;
    while (dx <= dy) {
        // Optimization opportunity: when dy is not moving, the same pixels
        // are drawn again and again.
        pbio_image_draw_hline(image, x - dx, y + dy, 1 + 2 * dx, value);
        pbio_image_draw_hline(image, x - dx, y - dy, 1 + 2 * dx, value);
        if (dx != dy) {
            pbio_image_draw_hline(image, x - dy, y + dx, 1 + 2 * dy, value);
            pbio_image_draw_hline(image, x - dy, y - dx, 1 + 2 * dy, value);
        }
        if (err > 0) {
            dy--;
            err -= dy * 8;
        }
        dx++;
        err += 4 + dx * 8;
    }
}

/**
 * Draw a single glyph.
 * @param [in] image  Image to draw into.
 * @param [in] font   Font to use for drawing.
 * @param [in] glyph  Glyph to draw.
 * @param [in] x      X coordinate of the baseline.
 * @param [in] y      Y coordinate of the baseline.
 * @param [in] value  Pixel value.
 */
static void pbio_image_draw_text_glyph(pbio_image_t *image,
    const pbio_font_t *font, const pbio_font_glyph_t *glyph, int x, int y,
    uint8_t value) {
    const uint8_t *src = &font->data[glyph->data_index];
    uint8_t bits;
    for (int dy = 0; dy < glyph->height; dy++) {
        for (int dx = 0; dx < glyph->width; dx++) {
            if ((dx & 7) == 0) {
                bits = *src++;
            }
            if (bits & 128) {
                pbio_image_draw_pixel(image, x + glyph->left + dx,
                    y - glyph->top + dy, value);
            }
            bits <<= 1;
        }
    }
}

/**
 * Draw text.
 * @param [in] image     Image to draw into.
 * @param [in] font      Font to use for drawing.
 * @param [in] x         X coordinate of the baseline.
 * @param [in] y         Y coordinate of the baseline.
 * @param [in] text      Text string.
 * @param [in] text_len  Text length.
 * @param [in] value     Pixel value.
 *
 * Clipping: drawing is clipped to image dimensions.
 *
 * Text uses ASCII encoding. Newlines are handled, text is flush left.
 */
void pbio_image_draw_text(pbio_image_t *image, const pbio_font_t *font, int x,
    int y, const char *text, size_t text_len, uint8_t value) {
    int x_left = x;
    const char *p;
    char c, prev = 0;
    const pbio_font_glyph_t *g;
    const pbio_font_kerning_t *k;

    for (p = text; p != text + text_len; p++) {
        c = *p;
        if (c == '\n') {
            x = x_left;
            y += font->line_height;
        } else if (c >= font->first && c <= font->last) {
            g = &font->glyphs[c - font->first];

            /* Apply kerning. */
            if (font->kernings && prev) {
                for (k = &font->kernings[g->kerning_index];
                     k != &font->kernings[(g + 1)->kerning_index]; k++) {
                    if (prev == k->previous) {
                        x += k->kerning;
                        break;
                    }
                }
            }

            /* Draw glyph. */
            pbio_image_draw_text_glyph(image, font, g, x, y, value);

            /* Advance pen. */
            x += g->advance;
        }
        prev = c;
    }
}

/**
 * Determine the bounding box of a text to be drawn.
 * @param [in]  font      Font to use for drawing.
 * @param [in]  text      Text string.
 * @param [in]  text_len  Text length.
 * @param [out] rect      Rectangle covering the drawn text.
 *
 * Text uses ASCII encoding. Newlines are handled, text is flush left.
 */
void pbio_image_bbox_text(const pbio_font_t *font, const char *text,
    size_t text_len, pbio_image_rect_t *rect) {
    int x, y;
    int xmin, ymin, xmax, ymax;
    const char *p;
    char c, prev = 0;
    const pbio_font_glyph_t *g;
    const pbio_font_kerning_t *k;

    x = 0;
    y = 0;

    xmin = ymin = INT_MAX;
    xmax = ymax = -INT_MAX;

    for (p = text; p != text + text_len; p++) {
        c = *p;
        if (c == '\n') {
            x = 0;
            y += font->line_height;
        } else if (c >= font->first && c <= font->last) {
            g = &font->glyphs[c - font->first];

            /* Apply kerning. */
            if (font->kernings && prev) {
                for (k = &font->kernings[g->kerning_index];
                     k != &font->kernings[(g + 1)->kerning_index]; k++) {
                    if (prev == k->previous) {
                        x += k->kerning;
                        break;
                    }
                }
            }

            /* Update min/max. */
            if (xmin > x + g->left) {
                xmin = x + g->left;
            }
            if (xmax < x + g->left + g->width) {
                xmax = x + g->left + g->width;
            }
            if (ymin > y - g->top) {
                ymin = y - g->top;
            }
            if (ymax < y - g->top + g->height) {
                ymax = y - g->top + g->height;
            }

            /* Advance pen. */
            x += g->advance;
        }
        prev = c;
    }

    if (xmin > xmax) {
        xmin = ymin = xmax = ymax = 0;
    }
    rect->x = xmin;
    rect->y = ymin;
    rect->width = xmax - xmin;
    rect->height = ymax - ymin;
}

/**
 * Scroll the whole image up by a number of pixels.
 * @param [in] image  Image to scroll.
 * @param [in] n      Number of pixels to scroll.
 */
static void pbio_image_scroll_up(pbio_image_t *image, int n) {
    uint8_t *dst = image->pixels;
    uint8_t *src = image->pixels + n * image->stride;
    int y;
    for (y = 0; y < image->height - n; y++) {
        memcpy(dst, src, image->width);
        src += image->stride;
        dst += image->stride;
    }
    for (; y < image->height; y++) {
        memset(dst, 0, image->width);
        dst += image->stride;
    }
}

/**
 * Print text inside image, like in a terminal, scroll when bottom is reached.
 * @param [in] image     Image to draw into.
 * @param [in] text      Text string.
 * @param [in] text_len  Text length.
 *
 * Clipping: drawing is clipped to image dimensions.
 *
 * Text uses ASCII encoding. Newlines are handled, text is flush left.
 */
void pbio_image_print(pbio_image_t *image, const char *text, size_t text_len) {
    const char *p;
    int x, y_top;
    char c, prev = 0;
    const pbio_font_t *font;
    const pbio_font_glyph_t *g;
    const pbio_font_kerning_t *k;

    font = image->print_font;
    if (!font) {
        return;
    }
    x = image->print_x_left;
    y_top = image->print_y_top;

    for (p = text; p != text + text_len; p++) {
        c = *p;
        if (c == '\n') {
            x = 0;
            y_top += font->line_height;
        } else if (c >= font->first && c <= font->last) {
            g = &font->glyphs[c - font->first];

            /* Apply kerning. */
            if (font->kernings && prev) {
                for (k = &font->kernings[g->kerning_index];
                     k != &font->kernings[(g + 1)->kerning_index]; k++) {
                    if (prev == k->previous) {
                        x += k->kerning;
                        break;
                    }
                }
            }

            /* Wrap? */
            if (x + g->left + g->width > image->width) {
                x = 0;
                y_top += font->line_height;
            }

            /* Scroll? */
            if (y_top + font->top_max - g->top + g->height > image->height &&
                y_top >= font->line_height) {
                pbio_image_scroll_up(image, font->line_height);
                y_top -= font->line_height;
            }

            /* Draw glyph. */
            pbio_image_draw_text_glyph(image, font, g, x, y_top + font->top_max,
                image->print_value);

            /* Advance pen. */
            x += g->advance;
        }
        prev = c;
    }

    image->print_x_left = x;
    image->print_y_top = y_top;
}

/**
 * Print null terminated text inside image, like in a terminal, scroll when
 * bottom is reached.
 * @param [in] image     Image to draw into.
 * @param [in] text      Null terminated text string.
 *
 * Clipping: drawing is clipped to image dimensions.
 *
 * Text uses ASCII encoding. Newlines are handled, text is flush left.
 */
void pbio_image_print0(pbio_image_t *image, const char *text) {
    pbio_image_print(image, text, strlen(text));
}

/**
 * Print unsigned number inside image, like in a terminal, scroll when bottom is
 * reached.
 * @param [in] image   Image to draw into.
 * @param [in] number  Number.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_print_uint(pbio_image_t *image, uint32_t number) {
    char buf[10], *p;

    p = &buf[10];

    if (number == 0) {
        *--p = '0';
    } else {
        while (number) {
            uint32_t r = number / 10;
            uint32_t d = number - r * 10;
            *--p = d + '0';
            number = r;
        }
    }

    pbio_image_print(image, p, &buf[10] - p);
}

/**
 * Print signed number inside image, like in a terminal, scroll when bottom is
 * reached.
 * @param [in] image   Image to draw into.
 * @param [in] number  Number.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_print_int(pbio_image_t *image, int32_t number) {
    if (number < 0) {
        pbio_image_print(image, "-", 1);
        number = -number;
    }
    pbio_image_print_uint(image, number);
}

/**
 * Print unsigned number inside image as hexadecimal, like in a terminal,
 * scroll when bottom is reached.
 * @param [in] image   Image to draw into.
 * @param [in] number  Number.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
void pbio_image_print_hex(pbio_image_t *image, uint32_t number) {
    char buf[8], *p;

    p = &buf[8];

    if (number == 0) {
        *--p = '0';
    } else {
        while (number) {
            uint32_t d = number & 0xf;
            *--p = "0123456789abcdef"[d];
            number >>= 4;
        }
    }

    pbio_image_print(image, p, &buf[8] - p);
}

/**
 * Print formatted output inside image, like in a terminal, scroll when bottom
 * is reached.
 * @param [in] image  Image to draw into.
 * @param [in] fmt    Printf-like format string.
 * @return            Same as printf.
 *
 * Clipping: drawing is clipped to image dimensions.
 */
int pbio_image_printf(pbio_image_t *image, const char *fmt, ...) {
    va_list ap;
    int r;
    char buf[128];

    va_start(ap, fmt);
    r = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    // Not supposed to return an error, but there are bugged libc, so just in
    // case, stop here.
    if (r < 0) {
        return r;
    }

    // Truncate if too long.
    if (r >= (int)sizeof(buf)) {
        r = sizeof(buf);
    }

    pbio_image_print(image, buf, r);

    return r;
}

#endif // PBIO_CONFIG_IMAGE
