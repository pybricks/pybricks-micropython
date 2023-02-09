// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup Geometry pbio/geometry: Linear algebra and geometry utilities
 *
 * Geometry utilities used by the pbio library.
 * @{
 */

#ifndef _PBIO_GEOMETRY_H_
#define _PBIO_GEOMETRY_H_

#include <stdint.h>
#include <stdbool.h>

#include <pbio/error.h>

/**
 * Coordinate-like type with x, y, and z floating point values.
 */
typedef struct _pbio_geometry_xyz_t {
    union {
        struct {
            float x; /**< X coordinate.*/
            float y; /**< Y coordinate.*/
            float z; /**< Z coordinate.*/
        };
        float values[3];
    };
} pbio_geometry_xyz_t;

/**
 * Representation of a 3x3 matrix.
 */
typedef struct _pbio_geometry_matrix_3x3_t {
    union {
        struct {
            float m11;
            float m12;
            float m13;
            float m21;
            float m22;
            float m23;
            float m31;
            float m32;
            float m33;
        };
        float values[9];
    };
} pbio_geometry_matrix_3x3_t;

void pbio_geometry_vector_normalize(pbio_geometry_xyz_t *input, pbio_geometry_xyz_t *output);

void pbio_geometry_vector_cross_product(pbio_geometry_xyz_t *a, pbio_geometry_xyz_t *b, pbio_geometry_xyz_t *output);

void pbio_geometry_vector_map(pbio_geometry_matrix_3x3_t *map, pbio_geometry_xyz_t *input, pbio_geometry_xyz_t *output);

pbio_error_t pbio_geometry_map_from_base_axes(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis, pbio_geometry_matrix_3x3_t *rotation);

#endif // _PBIO_GEOMETRY_H_

/** @} */
