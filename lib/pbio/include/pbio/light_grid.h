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

typedef struct _pbio_light_grid_t pbio_light_grid_t;

#if PBIO_CONFIG_LIGHT_GRID

extern const uint8_t pbio_light_grid_sys_pattern[1000];

/**
 * Get the light grid device
 * @param [out] light_grid  The light grid instance
 * @return                  ::PBIO_SUCCESS on success ::PBIO_ERROR_AGAIN if the
 *                          light grid is not ready for use yet or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_light_grid_get_dev(pbio_light_grid_t **light_grid);

/**
 * Get the size of the light grid
 * @param [in]  light_grid  The light grid instance
 * @return                  grid size
 */
uint8_t pbio_light_grid_get_size(pbio_light_grid_t *light_grid);

/**
 * Sets the pixels of all rows bitwise
 * @param [in]  light_grid  The light grid instance
 * @param [in]  rows        Rows of bytes. Each byte is one row, LSB right.
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_light_grid_set_rows(pbio_light_grid_t *light_grid, const uint8_t *rows);

/**
 * Sets the pixel to a given brightness.
 * @param [in]  light_grid  The light grid instance
 * @param [in]  row         Row index (0--m)
 * @param [in]  col         Column index (0--n)
 * @param [in]  brightness  Brightness (0--100)
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_light_grid_set_pixel(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness);

/**
 * Sets the pixel to a given brightness.
 * @param [in]  light_grid  The light grid instance
 * @param [in]  image       Buffer of brightness values (0--100)
 * @return                  ::PBIO_SUCCESS on success or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_light_grid_set_image(pbio_light_grid_t *light_grid, const uint8_t *image);

/**
 * Sets up the poller to display a series of frames
 * @param [in]  light_grid  The light grid instance
 * @param [in]  images      Buffer of buffer of brightness values (0--100)
 * @param [in]  frames      Number of images
 * @param [in]  interval    Time between subsequent images
 */
void pbio_light_grid_start_pattern(pbio_light_grid_t *light_grid, const uint8_t *images, uint8_t frames, uint32_t interval);

/**
 * Stops the pattern from updating further
 * @param [in]  light_grid  The light grid instance
 */
void pbio_light_grid_stop_pattern(pbio_light_grid_t *light_grid);

#endif // PBIO_CONFIG_LIGHT_GRID

#endif // _PBIO_LIGHT_GRID_H_

/** @} */
