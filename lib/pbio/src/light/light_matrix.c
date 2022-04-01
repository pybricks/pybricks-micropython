// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHT_MATRIX

#include <stdbool.h>

#include <pbio/error.h>
#include <pbio/light_matrix.h>
#include <pbio/util.h>

#include "animation.h"
#include "light_matrix.h"

/**
 * Sets the pixel to a given brightness.
 *
 * @param [in]  light_matrix  The light matrix instance
 * @param [in]  row         Row index (0 to size-1)
 * @param [in]  col         Column index (0 to size-1)
 * @param [in]  brightness  Brightness (0 to 100)
 * @return                  ::PBIO_SUCCESS on success or an
 *                          implementation-specific error on failure.
 */
static pbio_error_t _pbio_light_matrix_set_pixel(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightness) {
    uint8_t size = light_matrix->size;
    if (row >= size || col >= size) {
        return PBIO_SUCCESS;
    }

    // Rotate user input based on screen orientation
    switch (light_matrix->up_side) {
        case PBIO_ORIENTATION_SIDE_TOP:
        case PBIO_ORIENTATION_SIDE_FRONT:
            break;
        case PBIO_ORIENTATION_SIDE_LEFT: {
            uint8_t _col = col;
            col = row;
            row = size - 1 - _col;
            break;
        }
        case PBIO_ORIENTATION_SIDE_BOTTOM:
        case PBIO_ORIENTATION_SIDE_BACK:
            col = size - 1 - col;
            row = size - 1 - row;
            break;
        case PBIO_ORIENTATION_SIDE_RIGHT: {
            uint8_t _col = col;
            col = size - 1 - row;
            row = _col;
            break;
        }
    }

    // Set the pixel brightness
    return light_matrix->funcs->set_pixel(light_matrix, row, col, brightness);
}

/**
 * Initializes the required fields in a ::pbio_light_matrix_t.
 *
 * This function must be called before using the ::pbio_light_matrix_t.
 *
 * @param [in]  light_matrix  The struct to initialize.
 * @param [in]  size        The size of the light matrix.
 * @param [in]  funcs       The instance-specific callback functions.
 */
void pbio_light_matrix_init(pbio_light_matrix_t *light_matrix, uint8_t size, const pbio_light_matrix_funcs_t *funcs) {
    light_matrix->size = size;
    light_matrix->funcs = funcs;
    pbio_light_animation_init(&light_matrix->animation, NULL);
}

/**
 * Sets the light matrix orientation.
 *
 * The current display will not be affected. The new orientation will only be
 * applied to subsequent commands. This includes the next frame of an animation
 * if there is an animation currently running in the background.
 *
 * @param [in]  light_matrix    The light matrix.
 * @param [in]  up_side         Orientation of the light matrix: which side is up.
 */
void pbio_light_matrix_set_orientation(pbio_light_matrix_t *light_matrix, pbio_orientation_side_t up_side) {
    light_matrix->up_side = up_side;
}

/**
 * Gets the size of the light matrix.
 *
 * Light matrices are square, so this is equal to both the number of rows and the
 * number of columns in the matrix.
 *
 * @param [in]  light_matrix  The light matrix instance.
 * @return                    The matrix size.
 */
uint8_t pbio_light_matrix_get_size(pbio_light_matrix_t *light_matrix) {
    return light_matrix->size;
}

/**
 * Clears all pixels.
 *
 * If an animation is running in the background, it will be stopped.
 *
 * @param [in]  light_matrix  The light matrix instance
 * @return                  ::PBIO_SUCCESS on success or implementation-specific
 *                          error on failure.
 */
pbio_error_t pbio_light_matrix_clear(pbio_light_matrix_t *light_matrix) {
    pbio_light_matrix_stop_animation(light_matrix);
    for (uint8_t i = 0; i < light_matrix->size; i++) {
        for (uint8_t j = 0; j < light_matrix->size; j++) {
            pbio_error_t err = _pbio_light_matrix_set_pixel(light_matrix, i, j, 0);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

/**
 * Sets the pixels of all rows bitwise.
 *
 * Each bit in a byte sets a pixel in a row, where 1 means a pixel is on and 0
 * is off. The least significant bit is the right-most pixel. This is the same
 * format as used by the micro:bit.
 *
 * @p row 0 is the top row and @p col 0 is the left-most column of the matrix
 * according to the orientation set by pbio_light_matrix_set_orientation().
 *
 * If an animation is running in the background, it will be stopped.
 *
 * @param [in]  light_matrix  The light matrix instance
 * @param [in]  rows        Array of size bytes. Each byte is one row, LSB right.
 * @return                  ::PBIO_SUCCESS on success or implementation-specific
 *                          error on failure.
 */
pbio_error_t pbio_light_matrix_set_rows(pbio_light_matrix_t *light_matrix, const uint8_t *rows) {
    pbio_light_matrix_stop_animation(light_matrix);
    // Loop through all rows i, starting at row 0 at the top.
    uint8_t size = light_matrix->size;
    for (uint8_t i = 0; i < size; i++) {
        // Loop through all columns j, starting at col 0 on the left.
        for (uint8_t j = 0; j < size; j++) {
            // The pixel is on if the bit is high.
            bool on = rows[i] & (1 << (size - 1 - j));
            // Set the pixel.
            pbio_error_t err = _pbio_light_matrix_set_pixel(light_matrix, i, j, on * 100);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

/**
 * Sets the pixel to a given brightness.
 *
 * @p row 0 is the top row and @p col 0 is the left-most column of the matrix
 * according to the orientation set by pbio_light_matrix_set_orientation().
 *
 * If an animation is running in the background, it will be stopped and the
 * display is cleared.
 *
 * If @p row or @p col is out if range, nothing will happen (returns ::PBIO_SUCCESS).
 *
 * @param [in]  light_matrix  The light matrix instance
 * @param [in]  row         Row index (0 to size-1)
 * @param [in]  col         Column index (0 to size-1)
 * @param [in]  brightness  Brightness (0 to 100)
 * @return                  ::PBIO_SUCCESS on success or an
 *                          implementation-specific error on failure.
 */
pbio_error_t pbio_light_matrix_set_pixel(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightness) {

    if (pbio_light_animation_is_started(&light_matrix->animation)) {
        pbio_light_matrix_clear(light_matrix);
    }
    return _pbio_light_matrix_set_pixel(light_matrix, row, col, brightness);
}

/**
 * Sets the brightness of all pixels in the matrix to display an image.
 *
 * The @p image buffer is a two dimensional array of pbio_light_matrix_get_size()
 * rows and columns.
 *
 * @p row 0 is the top row and @p col 0 is the left-most column of the matrix
 * according to the orientation set by pbio_light_matrix_set_orientation().
 *
 * If an animation is running in the background, it will be stopped.
 *
 * @param [in]  light_matrix  The light matrix instance
 * @param [in]  image       Buffer of brightness values (0 to 100)
 * @return                  ::PBIO_SUCCESS on success or implementation-specific
 *                          error on failure.
 */
pbio_error_t pbio_light_matrix_set_image(pbio_light_matrix_t *light_matrix, const uint8_t *image) {
    pbio_light_matrix_stop_animation(light_matrix);
    uint8_t size = light_matrix->size;
    for (uint8_t r = 0; r < size; r++) {
        for (uint8_t c = 0; c < size; c++) {
            pbio_error_t err = _pbio_light_matrix_set_pixel(light_matrix, r, c, image[r * size + c]);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

static uint32_t pbio_light_matrix_animation_next(pbio_light_animation_t *animation) {
    pbio_light_matrix_t *light_matrix = PBIO_CONTAINER_OF(animation, pbio_light_matrix_t, animation);

    // display the current cell
    uint8_t size = light_matrix->size;
    const uint8_t *cell = light_matrix->animation_cells + size * size * light_matrix->current_cell;

    for (uint8_t r = 0; r < size; r++) {
        for (uint8_t c = 0; c < size; c++) {
            _pbio_light_matrix_set_pixel(light_matrix, r, c, cell[r * size + c]);
        }
    }

    // move to the next cell
    if (++light_matrix->current_cell >= light_matrix->num_animation_cells) {
        light_matrix->current_cell = 0;
    }

    return light_matrix->interval;
}

/**
 * Starts animating the light matrix in the background.
 *
 * If another animation is already running in the background, it will be stopped.
 *
 * @param [in]  light_matrix  The light matrix instance
 * @param [in]  cells       Array of size x size arrays of brightness values (0 to 100).
 * @param [in]  num_cells   Number of @p cells
 * @param [in]  interval    Time in milliseconds to wait between each cell.
 */
void pbio_light_matrix_start_animation(pbio_light_matrix_t *light_matrix, const uint8_t *cells, uint8_t num_cells, uint16_t interval) {
    pbio_light_matrix_stop_animation(light_matrix);

    pbio_light_animation_init(&light_matrix->animation, pbio_light_matrix_animation_next);
    light_matrix->animation_cells = cells;
    light_matrix->num_animation_cells = num_cells;
    light_matrix->interval = interval;
    light_matrix->current_cell = 0;

    pbio_light_animation_start(&light_matrix->animation);
}

/**
 * Stops the background animation.
 * @param [in]  light_matrix  The light matrix instance
 */
void pbio_light_matrix_stop_animation(pbio_light_matrix_t *light_matrix) {
    if (pbio_light_animation_is_started(&light_matrix->animation)) {
        pbio_light_animation_stop(&light_matrix->animation);
    }
}

#endif // PBIO_CONFIG_LIGHT_MATRIX
