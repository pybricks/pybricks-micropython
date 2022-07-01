// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>

#include <pbio/angle.h>

// Millidegrees per rotation
#define MDEG_PER_ROT (360000)

// Maximum number of rotations that still fit in a 31 bit millidegree value.
#define SMALL_ROT_MAX (INT32_MAX / MDEG_PER_ROT / 2)

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

void pbio_angle_diff(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result) {
    result->rotations = a->rotations - b->rotations;
    result->millidegrees = a->millidegrees - b->millidegrees;
    pbio_angle_flush(result);
}

int32_t pbio_angle_diff_mdeg(pbio_angle_t *a, pbio_angle_t *b) {
    return (a->rotations - b->rotations) * MDEG_PER_ROT + a->millidegrees - b->millidegrees;
}

bool pbio_angle_diff_is_small(pbio_angle_t *a, pbio_angle_t *b) {
    // Compute the full difference, and flush to whole rotations if possible.
    pbio_angle_t diff;
    pbio_angle_diff(a, b, &diff);

    // Return true if the rotation component is small enough.
    return diff.rotations < SMALL_ROT_MAX && diff.rotations > -SMALL_ROT_MAX;
}

void pbio_angle_sum(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result) {
    result->rotations = a->rotations + b->rotations;
    result->millidegrees = a->millidegrees + b->millidegrees;
    pbio_angle_flush(result);
}

void pbio_angle_avg(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result) {
    pbio_angle_sum(a, b, result);
    result->millidegrees = result->millidegrees / 2 + (result->rotations % 2) * MDEG_PER_ROT / 2;
    result->rotations /= 2;
}

void pbio_angle_add_mdeg(pbio_angle_t *a, int32_t increment) {
    pbio_angle_flush(a);
    a->millidegrees += increment;
}

void pbio_angle_neg(pbio_angle_t *a) {
    a->millidegrees *= -1;
    a->rotations *= -1;
}

int32_t pbio_angle_to_low_res(pbio_angle_t *a, int32_t scale) {

    // Fail safely on zero division.
    if (scale < 1) {
        return 0;
    }

    // Scale down rotations component.
    int32_t rotations_scaled = a->rotations * (MDEG_PER_ROT / scale);

    // The aforementioned output has this round off error in millidegrees.
    int32_t roundoff_mdeg = a->rotations * (MDEG_PER_ROT % scale);

    // Add scaled millidegree and roundoff component.
    return rotations_scaled + (a->millidegrees + roundoff_mdeg) / scale;
}

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
