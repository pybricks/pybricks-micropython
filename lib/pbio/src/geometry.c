// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include <assert.h>

#include <math.h>

#include <pbio/geometry.h>

/**
 * Gets @p index and @p sign of the axis that passes through given @p side of
 * a box.
 * @param [in]  side    The requested side of the box.
 * @param [out] index   The index 0 (x), 1 (y), or 2 (z) of the axis.
 * @param [out] sign    The sign of the axis: 1 or -1.
 */
void pbio_geometry_side_get_axis(pbio_geometry_side_t side, uint8_t *index, int8_t *sign) {
    *index = side & 0x03;
    *sign = (side & 0x04) ? -1 : 1;
}

/**
 * Given index and sign of two base axes, finds the axis to complete a right
 * handed coordinate system.
 * @param [in] index   Array of three axis indexes. First and last are given,
 *                     middle axis index is computed.
 * @param [in] sign    Array of three axis signs. First and last are given,
 *                     middle axis sign is computed.
 */
void pbio_geometry_get_complementary_axis(uint8_t *index, int8_t *sign) {

    // Inputs must have valid axis index and sign.
    assert(index[0] < 3);
    assert(index[2] < 3);
    assert(sign[0] == 1 || sign[2] == 1);

    // Two axis cannot be parallel.
    assert(index[2] != index[0]);

    // The final axis is determined as the cross product v_1 = v_2 (x) v_0.
    // Since we know the axes are just base axis up to a sign, the result C
    // simply has the index that isn't used yet:
    index[1] = 3 - index[2] - index[0];

    // We still have so evaluate the cross product to get the sign. With inputs
    // j = {0, 2}, both input vectors v_0 and v_2 can be written in
    // terms of known information as:
    //
    //                 [ i_j == 0 ]
    //    v_j = s_j *  [ i_j == 1 ]
    //                 [ i_j == 2 ]
    //
    // where s_j is the sign of vector j, and i_j is the axis index of j.
    //
    // For example, if the first input has sign -1 and index 2, it is
    //
    //         [ 0]
    //   v_0 = [ 0]
    //         [-1]
    //
    // So it is the negative Z vector.
    //
    // Given the vectors v_0 and v_2, we evaluate the cross product formula:
    //
    // v_1 = + s_0 * s_2 * ((i_2==1)*(i_0==2)-(i_0==1)*(i_2==2)) * E_0
    //       - s_0 * s_2 * ((i_2==0)*(i_0==2)-(i_0==0)*(i_2==2)) * E_1
    //       + s_0 * s_2 * ((i_2==0)*(i_0==1)-(i_0==0)*(i_2==1)) * E_2
    //
    // But we already know which of these three vectors is nonzero, so we just
    // need to take the scalar value in front of it. This leaves us with one of
    // three results. Since the formulas are so similar, we can generalize it
    // to one result that depends on the known final axis index. This gives:
    sign[1] = sign[0] * sign[2] * (
        (index[0] == (index[1] + 2) % 3) * (index[2] == (index[1] + 1) % 3) -
        (index[0] == (index[1] + 1) % 3) * (index[2] == (index[1] + 2) % 3)
        );
}

/**
 * Gets the side of a unit-sized box through which a given vector passes first.
 *
 * @param [in]  vector  The input vector.
 * @return              The side through which the vector passes first.
 */
pbio_geometry_side_t pbio_geometry_side_from_vector(pbio_geometry_xyz_t *vector) {

    // Find index and sign of maximum component
    float abs_max = 0;
    uint8_t axis = 0;
    bool negative = true;
    for (uint8_t i = 0; i < 3; i++) {
        if (vector->values[i] > abs_max) {
            abs_max = vector->values[i];
            negative = false;
            axis = i;
        } else if (-vector->values[i] > abs_max) {
            abs_max = -vector->values[i];
            negative = true;
            axis = i;
        }
    }

    // Return as side enum value.
    return axis | (negative << 2);
}

/**
 * Gets the norm of a vector.
 *
 * @param [in]  input   The vector.
 * @return              The norm of the vector.
 */
float pbio_geometry_vector_norm(pbio_geometry_xyz_t *input) {
    return sqrtf(input->x * input->x + input->y * input->y + input->z * input->z);
}

/**
 * Normalizes a vector so it has unit length.
 *
 * Output is allowed to be same as input, in which case input is normalized.
 *
 * @param [in]  input   The vector to normalize.
 * @param [out] output  The normalized vector.
 * @return              ::PBIO_ERROR_INVALID_ARG if the input has zero length, otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_geometry_vector_normalize(pbio_geometry_xyz_t *input, pbio_geometry_xyz_t *output) {

    // Compute the norm.
    float norm = pbio_geometry_vector_norm(input);

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
 * Multiplies two 3x3 matrices.
 *
 * @param [in]  a       The first 3x3 matrix.
 * @param [in]  b       The second 3x3 matrix.
 * @param [out] output  The resulting 3x3 matrix after multiplication.
 */
void pbio_geometry_matrix_multiply(pbio_geometry_matrix_3x3_t *a, pbio_geometry_matrix_3x3_t *b, pbio_geometry_matrix_3x3_t *output) {
    output->m11 = a->m11 * b->m11 + a->m12 * b->m21 + a->m13 * b->m31;
    output->m12 = a->m11 * b->m12 + a->m12 * b->m22 + a->m13 * b->m32;
    output->m13 = a->m11 * b->m13 + a->m12 * b->m23 + a->m13 * b->m33;
    output->m21 = a->m21 * b->m11 + a->m22 * b->m21 + a->m23 * b->m31;
    output->m22 = a->m21 * b->m12 + a->m22 * b->m22 + a->m23 * b->m32;
    output->m23 = a->m21 * b->m13 + a->m22 * b->m23 + a->m23 * b->m33;
    output->m31 = a->m31 * b->m11 + a->m32 * b->m21 + a->m33 * b->m31;
    output->m32 = a->m31 * b->m12 + a->m32 * b->m22 + a->m33 * b->m32;
    output->m33 = a->m31 * b->m13 + a->m32 * b->m23 + a->m33 * b->m33;
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

/**
 * Computes a rotation matrix for a quaternion.
 *
 * @param [in]  q       The quaternion.
 * @param [out] R       The rotation matrix.
 */
void pbio_geometry_quaternion_to_rotation_matrix(pbio_geometry_quaternion_t *q, pbio_geometry_matrix_3x3_t *R) {
    R->m11 = 1 - 2 * (q->q2 * q->q2 + q->q3 * q->q3);
    R->m21 = 2 * (q->q1 * q->q2 + q->q3 * q->q4);
    R->m31 = 2 * (q->q1 * q->q3 - q->q2 * q->q4);
    R->m12 = 2 * (q->q1 * q->q2 - q->q3 * q->q4);
    R->m22 = 1 - 2 * (q->q1 * q->q1 + q->q3 * q->q3);
    R->m32 = 2 * (q->q2 * q->q3 + q->q1 * q->q4);
    R->m13 = 2 * (q->q1 * q->q3 + q->q2 * q->q4);
    R->m23 = 2 * (q->q2 * q->q3 - q->q1 * q->q4);
    R->m33 = 1 - 2 * (q->q1 * q->q1 + q->q2 * q->q2);
}

/**
 * Computes a quaternion from a gravity unit vector.
 *
 * @param [in]     g  The gravity unit vector.
 * @param [out]    q  The resulting quaternion.
 */
void pbio_geometry_quaternion_from_gravity_unit_vector(pbio_geometry_xyz_t *g, pbio_geometry_quaternion_t *q) {
    if (g->z >= 0) {
        q->q4 = sqrtf((g->z + 1) / 2);
        q->q1 = g->y / sqrtf(2 * (g->z + 1));
        q->q2 = -g->x / sqrtf(2 * (g->z + 1));
        q->q3 = 0;
    } else {
        q->q4 = -g->y / sqrtf(2 * (1 - g->z));
        q->q1 = -sqrtf((1 - g->z) / 2);
        q->q2 = 0;
        q->q3 = -g->x / sqrtf(2 * (1 - g->z));
    }
}

/**
 * Computes the rate of change of a quaternion given the angular velocity vector.
 *
 * @param [in]     q                 Quaternion of the current orientation.
 * @param [in]     angular_velocity  The angular velocity vector in the body frame.
 * @param [out]    dq                The rate of change of the quaternion.
 */
void pbio_geometry_quaternion_get_rate_of_change(pbio_geometry_quaternion_t *q, pbio_geometry_xyz_t *angular_velocity, pbio_geometry_quaternion_t *dq) {

    pbio_geometry_xyz_t w = {
        .x = pbio_geometry_degrees_to_radians(angular_velocity->x),
        .y = pbio_geometry_degrees_to_radians(angular_velocity->y),
        .z = pbio_geometry_degrees_to_radians(angular_velocity->z),
    };

    dq->q1 = 0.5f * (w.z * q->q2 - w.y * q->q3 + w.x * q->q4);
    dq->q2 = 0.5f * (-w.z * q->q1 + w.x * q->q3 + w.y * q->q4);
    dq->q3 = 0.5f * (w.y * q->q1 - w.x * q->q2 + w.z * q->q4);
    dq->q4 = 0.5f * (-w.x * q->q1 - w.y * q->q2 - w.z * q->q3);
}

/**
 * Normalizes a quaternion so it has unit length.
 *
 * @param [inout] q  The quaternion to normalize.
 */
void pbio_geometry_quaternion_normalize(pbio_geometry_quaternion_t *q) {
    float norm = sqrtf(q->q1 * q->q1 + q->q2 * q->q2 + q->q3 * q->q3 + q->q4 * q->q4);

    if (norm < 0.0001f && norm > -0.0001f) {
        return;
    }

    q->q1 /= norm;
    q->q2 /= norm;
    q->q3 /= norm;
    q->q4 /= norm;
}

/**
 * Returns the maximum of two floating-point numbers.
 *
 * @param a The first floating-point number.
 * @param b The second floating-point number.
 * @return  The maximum of the two floating-point numbers.
 */
float pbio_geometry_maxf(float a, float b) {
    return a > b ? a : b;
}

/**
 * Returns the absolute value of a floating-point number.
 *
 * @param a The floating-point number.
 * @return  The absolute value of the floating-point number.
 */
float pbio_geometry_absf(float a) {
    return a < 0 ? -a : a;
}
