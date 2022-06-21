// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBIO_ANGLE_H_
#define _PBIO_ANGLE_H_

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

/**
 * Negates existing angle as: a = -a
 *
 * @param       a    Pointer to angle a.
 */
void pbio_angle_neg(pbio_angle_t *a);

/**
 * Gets the angular difference as: result = a - b.
 *
 * @param [in]  a       Pointer to angle a.
 * @param [in]  b       Pointer to angle b.
 * @param [out] result  Pointer to result.
 */
void pbio_angle_diff(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);

/**
 * Gets the angular sum as: result = a + b.
 *
 * @param [in]  a       Pointer to angle a.
 * @param [in]  b       Pointer to angle b.
 * @param [out] result  Pointer to result.
 */
void pbio_angle_sum(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);

/**
 * Gets the total angle (including whole rotations) expressed in millidegrees.
 * This should only be used for angles known to be less than 5965 rotations.
 *
 * @param [in]  a       Pointer to angle a.
 * @return int32_t      Millidegrees rotated.
 */
int32_t pbio_angle_get_mdeg(pbio_angle_t *a);

/**
 * Populates an angle object from a given number of whole degrees.
 *
 * @param [in]  a       Pointer to angle a.
 * @param [in]  b       Pointer to angle b.
 * @param [out] result  Pointer to result.
 */
void pbio_angle_from_deg(pbio_angle_t *a, int32_t degrees);

#endif // _PBIO_ANGLE_H_
