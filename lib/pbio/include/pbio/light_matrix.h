// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup LightMatrix pbio/light_matrix: Light matrix functions
 * @{
 */

#ifndef _PBIO_LIGHT_MATRIX_H_
#define _PBIO_LIGHT_MATRIX_H_

#include <stdint.h>

#include <pbdrv/led.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/geometry.h>
#include <pbio/light_animation.h>

/** A light matrix instance. */
typedef struct {
    /** Animation instance for background animation. */
    pbio_light_animation_t animation;
    /** Animation cell data. */
    const uint8_t *animation_cells;
    /** The number of cells in @p animation_cells */
    uint8_t num_animation_cells;
    /** The index of the currently displayed animation cell. */
    uint8_t current_cell;
    /** Animation update rate in milliseconds. */
    uint16_t interval;
    /** Size of the matrix (assumes matrix is square). */
    uint8_t size;
    /** Orientation of the matrix: which side is "up". */
    pbio_geometry_side_t up_side;
    /** The driver for this light matrix. */
    pbdrv_led_array_dev_t *led_array_dev;
} pbio_light_matrix_t;

#if PBIO_CONFIG_LIGHT_MATRIX

pbio_error_t pbio_light_matrix_get_dev(uint8_t index, uint8_t size, pbio_light_matrix_t **light_matrix);
uint8_t pbio_light_matrix_get_size(pbio_light_matrix_t *light_matrix);
void pbio_light_matrix_set_orientation(pbio_light_matrix_t *light_matrix, pbio_geometry_side_t up_side);
pbio_error_t pbio_light_matrix_clear(pbio_light_matrix_t *light_matrix);
pbio_error_t pbio_light_matrix_set_rows(pbio_light_matrix_t *light_matrix, const uint8_t *rows);
pbio_error_t pbio_light_matrix_set_pixel(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightness, bool clear_animation);
pbio_error_t pbio_light_matrix_set_image(pbio_light_matrix_t *light_matrix, const uint8_t *image);
void pbio_light_matrix_start_animation(pbio_light_matrix_t *light_matrix, const uint8_t *cells, uint8_t num_cells, uint16_t interval);
void pbio_light_matrix_stop_animation(pbio_light_matrix_t *light_matrix);

#else // PBIO_CONFIG_LIGHT_MATRIX

static inline pbio_error_t pbio_light_matrix_get_dev(uint8_t index, uint8_t size, pbio_light_matrix_t **light_matrix) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint8_t pbio_light_matrix_get_size(pbio_light_matrix_t *light_matrix) {
    return 0;
}

static inline void pbio_light_matrix_set_orientation(pbio_light_matrix_t *light_matrix, pbio_geometry_side_t up_side) {
}

static inline pbio_error_t pbio_light_matrix_clear(pbio_light_matrix_t *light_matrix) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_light_matrix_set_rows(pbio_light_matrix_t *light_matrix, const uint8_t *rows) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_light_matrix_set_pixel(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightness, bool clear_animation) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_light_matrix_set_image(pbio_light_matrix_t *light_matrix, const uint8_t *image) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_light_matrix_start_animation(pbio_light_matrix_t *light_matrix, const uint8_t *cells, uint8_t num_cells, uint16_t interval) {
}

static inline void pbio_light_matrix_stop_animation(pbio_light_matrix_t *light_matrix) {
}

#endif // PBIO_CONFIG_LIGHT_MATRIX

#endif // _PBIO_LIGHT_MATRIX_H_

/** @} */
