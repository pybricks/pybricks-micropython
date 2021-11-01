// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>

static pbio_error_t pbio_trajectory_patch(pbio_trajectory_t *trj, bool time_based, int32_t t0, int32_t duration, int32_t th3, int32_t wt, int32_t wmax, int32_t a) {

    // Get current reference point and acceleration, which will be the 0-point for the new trajectory

    pbio_trajectory_reference_t ref;
    pbio_trajectory_get_reference(trj, t0, &ref);

    int32_t th0 = ref.count;
    int32_t th0_ext = ref.count_ext;
    int32_t w0 = ref.rate;
    int32_t acceleration_ref = ref.acceleration;

    // First get the nominal commanded trajectory. This will be our default if we can't patch onto the existing one.
    pbio_error_t err;
    pbio_trajectory_t nominal;
    if (time_based) {
        err = pbio_trajectory_calc_angle_new(&nominal, t0, duration, th0, th0_ext, w0, wt, wmax, a);
    } else {
        err = pbio_trajectory_calc_time_new(&nominal, t0, th0, th3, w0, wt, wmax, a);
    }
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the reference acceleration equals the acceleration of the new nominal trajectory,
    // the trajectories are tangent at this point. Then we can patch the new trajectory
    // by letting its first segment be equal to the current segment of the ongoing trajectory.
    // This provides a seamless transition without having to resort to numerical tricks.
    if (acceleration_ref == nominal.a0) {
        // Find which section of the ongoing maneuver we were in, and take corresponding segment starting point
        if (t0 - trj->t1 < 0) {
            // We are still in the acceleration segment, so we can restart from its starting point
            t0 = trj->t0;
            w0 = trj->w0;
            th0 = trj->th0;
            th0_ext = trj->th0_ext;
        } else if (trj->forever || t0 - trj->t2 < 0) {
            // We are in the constant speed phase, so we can restart from its starting point
            t0 = trj->t1;
            w0 = trj->w1;
            th0 = trj->th1;
            th0_ext = trj->th1_ext;
        } else if (t0 - trj->t3 < 0) {
            // We are in the deceleration phase, so we can restart from its starting point
            t0 = trj->t2;
            w0 = trj->w1;
            th0 = trj->th2;
            th0_ext = trj->th2_ext;
        } else {
            // We are in the zero speed phase, so we can restart from its starting point
            t0 = trj->t3;
            w0 = 0;
            th0 = trj->th3;
            th0_ext = trj->th3_ext;
        }

        // We shifted the start time into the past, so we must adjust duration accordingly. But forever remains forever.
        if (duration != DURATION_FOREVER) {
            duration += (nominal.t0 - t0);
        }

        // Now we can make the new trajectory with a starting point coincident
        // with a point on the existing trajectory
        if (time_based) {
            return pbio_trajectory_calc_angle_new(trj, t0, duration, th0, th0_ext, w0, wt, wmax, a);
        } else {
            return pbio_trajectory_calc_time_new(trj, t0, th0, th3, w0, wt, wmax, a);
        }

    } else {
        // Trajectories were not tangent, so just return the nominal, unpatched trajectory
        *trj = nominal;
        return PBIO_SUCCESS;
    }
}

pbio_error_t pbio_trajectory_calc_angle_extend(pbio_trajectory_t *trj, int32_t t0, int32_t duration, int32_t wt, int32_t wmax, int32_t a) {
    return pbio_trajectory_patch(trj, true, t0, duration, 0, wt, wmax, a);
}

pbio_error_t pbio_trajectory_calc_time_extend(pbio_trajectory_t *trj, int32_t t0, int32_t th3, int32_t wt, int32_t wmax, int32_t a) {
    return pbio_trajectory_patch(trj, false, t0, 0, th3, wt, wmax, a);
}
