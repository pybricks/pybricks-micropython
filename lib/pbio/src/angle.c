// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>

#include <pbio/angle.h>
#include <pbio/int_math.h>

// Millidegrees per rotation
#define MDEG_PER_ROT (360000)

// Maximum number of rotations that still fit in a 30 bit millidegree value.
#define SMALL_ROT_MAX (INT32_MAX / 2 / MDEG_PER_ROT - 1)

void pbio_angle_flush(pbio_angle_t *a) {
    while (a->millidegrees > MDEG_PER_ROT) {
        a->millidegrees -= MDEG_PER_ROT;
        a->rotations += 1;
    }
    while (a->millidegrees < -MDEG_PER_ROT) {
        a->millidegrees += MDEG_PER_ROT;
        a->rotations -= 1;
    }
}

/**
 * Gets the angular difference as: result = a - b.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @param [out] result  Result.
 */
void pbio_angle_diff(const pbio_angle_t *a, const pbio_angle_t *b, pbio_angle_t *result) {
    result->rotations = a->rotations - b->rotations;
    result->millidegrees = a->millidegrees - b->millidegrees;
    pbio_angle_flush(result);
}

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
int32_t pbio_angle_diff_mdeg(const pbio_angle_t *a, const pbio_angle_t *b) {
    return (a->rotations - b->rotations) * MDEG_PER_ROT + a->millidegrees - b->millidegrees;
}

/**
 * Checks if difference (a - b) can fit in millidegrees alone.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @return              True if (a-b) in millidegrees is valid.
 */
bool pbio_angle_diff_is_small(const pbio_angle_t *a, const pbio_angle_t *b) {
    // Compute the full difference, and flush to whole rotations if possible.
    pbio_angle_t diff;
    pbio_angle_diff(a, b, &diff);

    // Return true if the rotation component is small enough.
    return diff.rotations < SMALL_ROT_MAX && diff.rotations > -SMALL_ROT_MAX;
}

/**
 * Gets the angular sum as: result = a + b.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @param [out] result  Result.
 */
void pbio_angle_sum(const pbio_angle_t *a, const pbio_angle_t *b, pbio_angle_t *result) {
    result->rotations = a->rotations + b->rotations;
    result->millidegrees = a->millidegrees + b->millidegrees;
    pbio_angle_flush(result);
}

/**
 * Gets the angular average as: result = (a + b) / 2.
 *
 * @param [in]  a       Angle a.
 * @param [in]  b       Angle b.
 * @param [out] result  Result.
 */
void pbio_angle_avg(const pbio_angle_t *a, const pbio_angle_t *b, pbio_angle_t *result) {
    pbio_angle_sum(a, b, result);
    result->millidegrees = result->millidegrees / 2 + (result->rotations % 2) * MDEG_PER_ROT / 2;
    result->rotations /= 2;
}

/**
 * Adds a given number of millidegrees to an existing angle.
 *
 * @param [in]  a         Angle a.
 * @param [in]  increment Millidegrees to add.
 */
void pbio_angle_add_mdeg(pbio_angle_t *a, int32_t increment) {
    pbio_angle_flush(a);
    a->millidegrees += increment;
}

/**
 * Negates existing angle as: a = -a
 *
 * @param       a    Angle a.
 */
void pbio_angle_neg(pbio_angle_t *a) {
    a->millidegrees *= -1;
    a->rotations *= -1;
}

/**
 * Scales down high resolution angle to single integer.
 *
 * For example, if scale is 1000, this converts the angle in
 * millidegrees to the value in degrees.
 *
 * @param [out]  a       Angle a.
 * @param [in]   scale   Ratio between high resolution angle and input.
 */
int32_t pbio_angle_to_low_res(const pbio_angle_t *a, int32_t scale) {

    // Fail safely on zero division.
    if (scale < 1) {
        return 0;
    }

    // Scale down rotations component.
    int32_t rotations_component = pbio_int_math_mult_then_div(a->rotations, MDEG_PER_ROT, scale);

    // Scale down millidegree component, rounded to nearest ouput unit.
    int32_t millidegree_component = (a->millidegrees + pbio_int_math_sign(a->millidegrees) * scale / 2) / scale;

    return rotations_component + millidegree_component;
}

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
void pbio_angle_from_low_res(pbio_angle_t *a, int32_t input, int32_t scale) {

    // Fail safely on zero division.
    if (scale < 1 || scale > MDEG_PER_ROT) {
        return;
    }

    // Get whole rotations.
    a->rotations = input / (MDEG_PER_ROT / scale);

    // The round off is the truncated part in user units.
    int32_t roundoff_user = input - a->rotations * (MDEG_PER_ROT / scale);

    // We'll keep that portion in the millidegrees component.
    a->millidegrees = roundoff_user * scale;
}
