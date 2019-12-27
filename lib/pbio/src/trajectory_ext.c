// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>

pbio_error_t pbio_trajectory_make_forever(int32_t t0, int32_t th0, int32_t w0, int32_t wt, int32_t wmax, int32_t a, pbio_control_trajectory_t *ref) {
    // For infinite maneuvers like RUN and RUN_STALLED, no end time is specified, so we take a
    // fictitious 60 seconds. This allows us to use the same code to get the trajectory for the
    // initial acceleration phase and the constant speed phase. Setting the forever flag allows
    // us to ignore the deceleration phase while getting the reference, hence moving forever.
    pbio_error_t err = pbio_trajectory_make_time_based(t0, t0 + 60*US_PER_SECOND, th0, w0, wt, wmax, a, ref);

    // This is an infinite maneuver
    ref->forever = true;

    return err;
}
