// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include <pbio/config.h>

#include <math.h>

#include <pbio/geometry.h>

/**
 * Normalizes a vector so it has unit length.
 *
 * @param [in]  input   The vector to normalize.
 * @param [out] output  The normalized vector.
 * @return              ::PBIO_ERROR_INVALID_ARG if the input has zero length, otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_geometry_vector_normalize(pbio_geometry_xyz_t *input, pbio_geometry_xyz_t *output) {

    // Compute the norm.
    float norm = sqrtf(input->x * input->x + input->y * input->y + input->z * input->z);

    // If the vector norm is zero, do nothing.
    if (norm == 0.0f) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Otherwise, normalize.
    output->x = input->x / norm;
    output->y = input->y / norm;
    output->z = input->z / norm;
    return PBIO_SUCCESS;
}

/**
 * Gets the cross product of two vectors.
 *
 * @param [in]  a       The first vector.
 * @param [in]  b       The first vector.
 * @param [out] output  The result.
 */
void pbio_geometry_vector_cross_product(pbio_geometry_xyz_t *a, pbio_geometry_xyz_t *b, pbio_geometry_xyz_t *output) {
    output->x = a->y * b->z - a->z * b->y;
    output->y = a->z * b->x - a->x * b->z;
    output->z = a->x * b->y - a->y * b->x;
}

/**
 * Gets the scalar projection of one vector onto the line spanned by another.
 *
 * @param [in]  axis        The axis on which to project.
 * @param [in]  input       The input vector.
 * @param [out] projection  The projection.
 * @return                  ::PBIO_ERROR_INVALID_ARG if the axis has zero length, otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_geometry_vector_project(pbio_geometry_xyz_t *axis, pbio_geometry_xyz_t *input, float *projection) {

    // Normalize the given axis so its magnitude does not matter.
    pbio_geometry_xyz_t unit_axis;
    pbio_error_t err = pbio_geometry_vector_normalize(axis, &unit_axis);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Compute the projection.
    *projection = unit_axis.x * input->x + unit_axis.y * input->y + unit_axis.z * input->z;
    return PBIO_SUCCESS;
}

/**
 * Maps the input vector using: output = map * input
 *
 * @param [in]  map     The 3x3 map, such as a rotation matrix.
 * @param [in]  input   The input vector
 * @param [out] output  The resulting output vector.
 */
void pbio_geometry_vector_map(pbio_geometry_matrix_3x3_t *map, pbio_geometry_xyz_t *input, pbio_geometry_xyz_t *output) {
    output->x = input->x * map->m11 + input->y * map->m12 + input->z * map->m13;
    output->y = input->x * map->m21 + input->y * map->m22 + input->z * map->m23;
    output->z = input->x * map->m31 + input->y * map->m32 + input->z * map->m33;
}

/**
 * Gets a mapping (a rotation matrix) from two orthogonal base axes.
 *
 * @param [in]  x_axis  The X axis. Need not be normalized.
 * @param [in]  z_axis  The Z axis. Need not be normalized.
 * @param [out] map     The completed map, including the computed y_axis.
 * @return              ::PBIO_ERROR_INVALID_ARG if the axes are zero or not orthogonal, otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_geometry_map_from_base_axes(pbio_geometry_xyz_t *x_axis, pbio_geometry_xyz_t *z_axis, pbio_geometry_matrix_3x3_t *map) {

    pbio_geometry_xyz_t x_axis_normal;
    pbio_error_t err = pbio_geometry_vector_normalize(x_axis, &x_axis_normal);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    pbio_geometry_xyz_t z_axis_normal;
    err = pbio_geometry_vector_normalize(z_axis, &z_axis_normal);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Assert that X and Z are orthogonal.
    float inner_product = x_axis_normal.x * z_axis_normal.x + x_axis_normal.y * z_axis_normal.y + x_axis_normal.z * z_axis_normal.z;
    if (inner_product > 0.001f || inner_product < -0.001f) {
        return PBIO_ERROR_INVALID_ARG;
    }

    pbio_geometry_xyz_t y_axis_normal;
    pbio_geometry_vector_cross_product(&z_axis_normal, &x_axis_normal, &y_axis_normal);

    // Build the resulting matrix as [x_axis y_axis z_axis]
    *map = (pbio_geometry_matrix_3x3_t) {
        .m11 = x_axis_normal.x, .m12 = y_axis_normal.x, .m13 = z_axis_normal.x,
        .m21 = x_axis_normal.y, .m22 = y_axis_normal.y, .m23 = z_axis_normal.y,
        .m31 = x_axis_normal.z, .m32 = y_axis_normal.z, .m33 = z_axis_normal.z,
    };

    return PBIO_SUCCESS;
}
