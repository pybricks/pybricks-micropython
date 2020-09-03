// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup Lightgrid Light grid functions
 * @{
 */

#ifndef _PBIO_LIGHT_GRID_H_
#define _PBIO_LIGHT_GRID_H_

#include <pbio/config.h>
#include <pbio/error.h>

/** A light grid instance. */
typedef struct _pbio_light_grid_t pbio_light_grid_t;

#if PBIO_CONFIG_LIGHT_GRID

extern const uint8_t pbio_light_grid_sys_pattern[1000];

pbio_error_t pbio_light_grid_get_dev(pbio_light_grid_t **light_grid);
uint8_t pbio_light_grid_get_size(pbio_light_grid_t *light_grid);
pbio_error_t pbio_light_grid_set_rows(pbio_light_grid_t *light_grid, const uint8_t *rows);
pbio_error_t pbio_light_grid_set_pixel(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness);
pbio_error_t pbio_light_grid_set_image(pbio_light_grid_t *light_grid, const uint8_t *image);
void pbio_light_grid_start_pattern(pbio_light_grid_t *light_grid, const uint8_t *images, uint8_t frames, uint32_t interval);
void pbio_light_grid_stop_pattern(pbio_light_grid_t *light_grid);

#endif // PBIO_CONFIG_LIGHT_GRID

#endif // _PBIO_LIGHT_GRID_H_

/** @} */
