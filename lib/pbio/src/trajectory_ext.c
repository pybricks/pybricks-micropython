// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>

pbio_error_t pbio_trajectory_make_time_based_patched(pbio_trajectory_t *ref, bool forever, int32_t t0, int32_t t3, int32_t th0, int32_t w0, int32_t wt, int32_t wmax, int32_t a, bool *patched) {
    // TODO: Patch new trajectory onto existing one when possible
    return pbio_trajectory_make_time_based(ref, forever, t0, t3, th0, w0, wt, wmax, a);
}
