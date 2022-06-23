// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

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

/**
 * Negates existing angle as: a = -a
 *
 * @param       a    Angle a.
 */
void pbio_angle_neg(pbio_angle_t *a);

/**
 * Gets the angular difference as: result = a - b.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @param [out] result  Result.
 */
void pbio_angle_diff(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);

/**
 * Gets the angular difference as: result = a - b, and scale to millidegrees.
 *
 * This should only be used for angles less than 5965 rotations apart which can
 * be checked with pbio_angle_diff_is_small if unknown.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @return int32_t      Difference in millidegrees.
 */
int32_t pbio_angle_diff_mdeg(pbio_angle_t *a, pbio_angle_t *b);

/**
 * Checks if difference (a - b) can fit in millidegrees alone.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @return              True if (a-b) in millidegrees is valid.
 */
bool pbio_angle_diff_is_small(pbio_angle_t *a, pbio_angle_t *b);

/**
 * Gets the angular sum as: result = a + b.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @param [out] result  Result.
 */
void pbio_angle_sum(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result);

/**
 * Adds a given number of millidegrees to an existing angle.
 *
 * @param [in]  a         Angle a.
 * @param [in]  increment Millidegrees to add.
 */
void pbio_angle_add_mdeg(pbio_angle_t *a, int32_t increment);

/**
 * Scales down high resolution angle to single integer.
 *
 * For example, if scale is 1000, this converts the angle in
 * millidegrees to the value in degrees.
 *
 * @param [out]  a       Angle a.
 * @param [in]   scale   Ratio between high resolution angle and input.
 */
int32_t pbio_angle_to_low_res(pbio_angle_t *a, int32_t scale);

/**
 * Populates object from scaled-up integer value.
 *
 * For example, if @p scale is 1000, this converts the @p input in
 * degrees to @p a in millidegrees.
 *
 * @param [out]  a       Angle a.
 * @param [in]   input   Value to convert.
 * @param [in]   scale   Ratio between high resolution angle and input.
 */
void pbio_angle_from_low_res(pbio_angle_t *a, int32_t input, int32_t scale);

#endif // _PBIO_ANGLE_H_
