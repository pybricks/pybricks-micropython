// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdint.h>

#include <pbdrv/led.h>

#include <pbio/error.h>
#include <pbio/light_matrix.h>

#include "animation.h"

#ifndef _PBIO_LIGHT_LIGHT_MATRIX_H_
#define _PBIO_LIGHT_LIGHT_MATRIX_H_

struct _pbio_light_matrix_t {
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
};

#endif // _PBIO_LIGHT_LIGHT_MATRIX_H_
