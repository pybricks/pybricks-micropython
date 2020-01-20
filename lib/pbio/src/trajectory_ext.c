// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>

static pbio_error_t pbio_trajectory_patch(pbio_trajectory_t *ref, bool time_based, bool forever, int32_t t0, int32_t t3, int32_t th3, int32_t wt, int32_t wmax, int32_t a) {

    // Get current reference point and acceleration, which will be the 0-point for the new trajectory
    int32_t th0;
    int32_t th0_ext;
    int32_t w0;
    int32_t acceleration_ref;
    pbio_trajectory_get_reference(ref, t0, &th0, &th0_ext, &w0, &acceleration_ref);

    // First get the nominal commanded trajectory. This will be our default if we can't patch onto the existing one.
    pbio_error_t err;
    pbio_trajectory_t nominal;
    err = pbio_trajectory_make_time_based(&nominal, forever, t0, t3, th0, th0_ext, w0, wt, wmax, a);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the reference acceleration equals the acceleration of the new nominal trajectory, 
    // the trajectories are tangent at this point. Then we can patch the new trajectory
    // by letting its first segment be equal to the current segment of the ongoing trajectory.
    // This provides a seamless transition without having to resort to numerical tricks.
    if (acceleration_ref == nominal.a0) {
        // Find which section of the ongoing maneuver we were in, and take corresponding segment starting point
        if (t0 - ref->t1 < 0) {
            // We are still in the acceleration segment, so we can restart from its starting point
            t0 = ref->t0;
            w0 = ref->w0;
            th0 = ref->th0;
            th0_ext = ref->th0_ext;
        } else if (ref->forever || t0 - ref->t2 < 0) {
            // We are in the constant speed phase, so we can restart from its starting point
            t0 = ref->t1;
            w0 = ref->w1;
            th0 = ref->th1;
            th0_ext = ref->th1_ext;
        } else if (t0 - ref->t3 < 0) {
            // We are in the deceleration phase, so we can restart from its starting point
            t0 = ref->t2;
            w0 = ref->w1;
            th0 = ref->th2;
            th0_ext = ref->th2_ext;
        }
        else {
            // We are in the zero speed phase, so we can restart from its starting point
            t0 = ref->t3;
            w0 = 0;
            th0 = ref->th3;
            th0_ext = ref->th3_ext;
        }
        // Now we can make the new trajectory with a starting point coincident
        // with a point on the existing trajectory
        return pbio_trajectory_make_time_based(ref, forever, t0, t3, th0, th0_ext, w0, wt, wmax, a);
    }
    else {
        // Trajectories were not tangent, so just return the nominal, unpatched trajectory
        *ref = nominal;
        return PBIO_SUCCESS;
    }
}

pbio_error_t pbio_trajectory_make_time_based_patched(pbio_trajectory_t *ref, bool forever, int32_t t0, int32_t t3, int32_t wt, int32_t wmax, int32_t a) {
    return pbio_trajectory_patch(ref, true, forever, t0, t3, 0, wt, wmax, a);
}
