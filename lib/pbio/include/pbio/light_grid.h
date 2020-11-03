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

#if PBIO_CONFIG_LIGHT_GRID

uint8_t pbio_light_grid_get_size(pbio_light_grid_t *light_grid);
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
