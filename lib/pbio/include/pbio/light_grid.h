// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup Lightgrid Light grid functions
 * @{
 */

#ifndef _PBIO_LIGHT_GRID_H_
#define _PBIO_LIGHT_GRID_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>

/** A light grid instance. */
typedef struct _pbio_light_grid_t pbio_light_grid_t;

// FIXME: Move to a common location. This is also relevant for IMU orientation.
typedef enum {
    PBIO_SIDE_TOP,       /**< The top side of a rectangular box or screen */
    PBIO_SIDE_LEFT,      /**< The left side of a rectangular box or screen */
    PBIO_SIDE_BOTTOM,    /**< The bottom side of a rectangular box or screen */
    PBIO_SIDE_RIGHT,     /**< The right side of a rectangular box or screen */
    PBIO_SIDE_FRONT,     /**< The front side of a rectangular box */
    PBIO_SIDE_BACK,      /**< The back side of a rectangular box */
} pbio_side_t;

#if PBIO_CONFIG_LIGHT_GRID

uint8_t pbio_light_grid_get_size(pbio_light_grid_t *light_grid);
void pbio_light_grid_set_orientation(pbio_light_grid_t *light_grid, pbio_side_t up_side);
pbio_error_t pbio_light_grid_set_rows(pbio_light_grid_t *light_grid, const uint8_t *rows);
pbio_error_t pbio_light_grid_set_pixel_user(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness);
pbio_error_t pbio_light_grid_set_image(pbio_light_grid_t *light_grid, const uint8_t *image);
void pbio_light_grid_start_animation(pbio_light_grid_t *light_grid, const uint8_t *cells, uint8_t num_cells, uint16_t interval);
void pbio_light_grid_stop_animation(pbio_light_grid_t *light_grid);

#else // PBIO_CONFIG_LIGHT_GRID

static inline uint8_t pbio_light_grid_get_size(pbio_light_grid_t *light_grid) {
    return 0;
}

static inline pbio_error_t pbio_light_grid_set_rows(pbio_light_grid_t *light_grid, const uint8_t *rows) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_light_grid_set_pixel(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_light_grid_set_image(pbio_light_grid_t *light_grid, const uint8_t *image) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_light_grid_start_animation(pbio_light_grid_t *light_grid, const uint8_t *cells, uint8_t num_cells, uint16_t interval) {
}

static inline void pbio_light_grid_stop_animation(pbio_light_grid_t *light_grid) {
}

#endif // PBIO_CONFIG_LIGHT_GRID

#endif // _PBIO_LIGHT_GRID_H_

/** @} */
