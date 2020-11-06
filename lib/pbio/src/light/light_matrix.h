// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/light_matrix.h>

#include "animation.h"

#ifndef _PBIO_LIGHT_LIGHT_MATRIX_H_
#define _PBIO_LIGHT_LIGHT_MATRIX_H_

/** Implementation-specific callbacks for a light matrix. */
typedef struct {
    /**
     * Sets the light at @p row, @p col to @p brightness.
     *
     * @param [in]  light_matrix  The light matrix instance.
     * @param [in]  row         The row index (0 to size-1).
     * @param [in]  col         The column index (0 to size-1).
     * @param [in]  brightess   The apparent brightness (0 to 100).
     * @return                  Success/failure of the operation.
     */
    pbio_error_t (*set_pixel)(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightess);
} pbio_light_matrix_funcs_t;

struct _pbio_light_matrix_t {
    /** Animation instance for background animation. */
    pbio_light_animation_t animation;
    /** Implementation specific callback functions. */
    const pbio_light_matrix_funcs_t *funcs;
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
    pbio_side_t up_side;
};

void pbio_light_matrix_init(pbio_light_matrix_t *light_matrix, uint8_t size, const pbio_light_matrix_funcs_t *funcs);

#endif // _PBIO_LIGHT_LIGHT_MATRIX_H_
