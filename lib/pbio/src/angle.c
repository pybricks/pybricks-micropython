// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/angle.h>

#define MDEG_PER_ROT (360000)

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

void pbio_angle_sum(pbio_angle_t *a, pbio_angle_t *b, pbio_angle_t *result) {
    result->rotations = a->rotations + b->rotations;
    result->millidegrees = a->millidegrees + b->millidegrees;
    pbio_angle_flush(result);
}

void pbio_angle_neg(pbio_angle_t *a) {
    a->millidegrees *= -1;
    a->rotations *= -1;
}

int32_t pbio_angle_get_mdeg(pbio_angle_t *a) {
    return a->rotations * MDEG_PER_ROT + a->millidegrees;
}

void pbio_angle_from_deg(pbio_angle_t *a, int32_t degrees) {
    a->rotations = degrees / 360;
    a->millidegrees = degrees % 360;
}
