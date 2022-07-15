// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup Angle pbio/angle: Long high resolution position type
 *
 * Type definition and math operations for long position type.
 * @{
 */

#ifndef _PBIO_ANGLE_H_
#define _PBIO_ANGLE_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Angle type for up to 2^31 rotations with millidegree resolution.
 * The total angle in millidegrees is:
 *
 *      rotations * 360 000 + millidegrees.
 *
 * The millidegree component may be positive or negative and may itself
 * span multiple rotations.
 */
typedef struct _pbio_angle_t {
    int32_t rotations;    /**< Whole rotations. */
    int32_t millidegrees; /**< Millidegrees.*/
} pbio_angle_t;

// Conversion to and from basic types:

int32_t pbio_angle_to_low_res(pbio_angle_t *a, int32_t scale);
void pbio_angle_from_low_res(pbio_angle_t *a, int32_t input, int32_t scale);

// Inplace operations on an angle:

void pbio_angle_neg(pbio_angle_t *a);
void pbio_angle_add_mdeg(pbio_angle_t *a, int32_t increment);

// Binary operations on two angles to produce new angle:

void pbio_angle_diff(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);
void pbio_angle_sum(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);
void pbio_angle_avg(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);

// Compare two angles:

int32_t pbio_angle_diff_mdeg(pbio_angle_t *a, pbio_angle_t *b);
bool pbio_angle_diff_is_small(pbio_angle_t *a, pbio_angle_t *b);

#endif // _PBIO_ANGLE_H_

/** @} */
