// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>

static int64_t as_mcount(int32_t count, int32_t count_ext) {
    return ((int64_t)count) * 1000 + count_ext;
}

static void as_count(int64_t mcount, int32_t *count, int32_t *count_ext) {
    *count = (int32_t)(mcount / 1000);
    *count_ext = mcount - ((int64_t)*count) * 1000;
}

void reverse_trajectory(pbio_trajectory_t *trj) {
    // Mirror angles about initial angle th0

    // First load as high res types
    int64_t mth0 = as_mcount(trj->th0, trj->th0_ext);
    int64_t mth1 = as_mcount(trj->th1, trj->th1_ext);
    int64_t mth2 = as_mcount(trj->th2, trj->th2_ext);
    int64_t mth3 = as_mcount(trj->th3, trj->th3_ext);

    // Perform the math
    mth1 = 2 * mth0 - mth1;
    mth2 = 2 * mth0 - mth2;
    mth3 = 2 * mth0 - mth3;

    // Store as simple type again
    as_count(mth1, &trj->th1, &trj->th1_ext);
    as_count(mth2, &trj->th2, &trj->th2_ext);
    as_count(mth3, &trj->th3, &trj->th3_ext);

    // Negate speeds and accelerations
    trj->w0 *= -1;
    trj->w1 *= -1;
    trj->a0 *= -1;
    trj->a2 *= -1;
}

void pbio_trajectory_make_constant(pbio_trajectory_t *trj, int32_t t0, int32_t th0, int32_t w3) {
    // All times equal to initial time:
    trj->t0 = t0;
    trj->t1 = t0;
    trj->t2 = t0;
    trj->t3 = t0;

    // All angles equal to initial angle:
    trj->th0 = th0;
    trj->th1 = th0;
    trj->th2 = th0;
    trj->th3 = th0;

    // FIXME: Angle based does not have high res yet
    trj->th0_ext = 0;
    trj->th1_ext = 0;
    trj->th2_ext = 0;
    trj->th3_ext = 0;

    // All speeds/accelerations zero:
    trj->w0 = 0;
    trj->w1 = 0;
    trj->a0 = 0;
    trj->a2 = 0;

    // Set final speed
    trj->w3 = w3;
}

static int64_t x_time(int32_t b, int32_t t) {
    return (((int64_t)b) * ((int64_t)t)) / US_PER_MS;
}

static int64_t x_time2(int32_t b, int32_t t) {
    return x_time(x_time(b, t), t) / (2 * US_PER_MS);
}

// Computes a trajectory for a timed command assuming *positive* speed
static void pbio_trajectory_new_forward_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // The given time constraints won't change, so store them right away.
    trj->th0 = c->th0;
    trj->t0 = c->t0;
    trj->t3 = c->t0 + c->duration;

    // Store target speed.
    trj->w3 = c->w3;

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is equal to or less than the target speed. Otherwise
    // it decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = c->w0 <= c->wt ? c->a0_abs : -c->a0_abs;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -c->a2_abs;

    // Bind the initial speed and target speeds to feasible regions such that
    // we can still decelerate to the final speed within the duration.
    int32_t wmin = -timest(c->a0_abs, c->duration);
    int32_t wmax = c->w3 + timest(c->a2_abs, c->duration);
    int32_t wt = min(c->wt, wmax);
    trj->w0 = min(c->w0, wmax);
    trj->w0 = max(trj->w0, wmin);
    trj->w3 = trj->w3 == 0 ? 0 : c->wt;

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt0 = c->duration;
    int32_t t3mt2;
    int32_t t2mt1;
    int32_t t1mt0;

    // The in-phase completes at the point where it reaches the target speed.
    t1mt0 = wdiva(wt - trj->w0, trj->a0);

    // The out-phase rotation is the time passed while slowing down.
    t3mt2 = wdiva(trj->w3 - wt, trj->a2);

    // In the decreasing case, we always reach the target speed, since the case
    // where it isn't reached is already handled above. For the increasing case
    // we still need to verify the feasibility. So as a starting point:
    t2mt1 = t3mt0 - t1mt0 - t3mt2;
    trj->w1 = wt;

    // The aforementioned results are valid for the increasing case only if
    // the computed time intervals don't overlap, so t2mt1 must be positive.
    if (trj->a0 > 0 && t2mt1 < 0) {
        // The result is invalid. Then we will not reach the target speed, but
        // begin decelerating at the point where the in/out phases intersect.
        // The valid result depends on whether there is an end speed or not.
        if (trj->w3 == 0) {
            t1mt0 = wdiva(trj->w0 + timest(trj->a2, t3mt0), trj->a2 - trj->a0);
        } else {
            t1mt0 = t3mt0;
            trj->w3 = trj->w0 + timest(trj->a0, t3mt0);
        }
        // In both cases, there is no constant speed phase.
        t2mt1 = 0;
        t3mt2 = t3mt0 - t1mt0;

        // The target speed is not reached; this is the best we can reach.
        trj->w1 = trj->w0 + timest(trj->a0, t1mt0);
    }

    // Store other results/arguments
    trj->t1 = c->t0 + t1mt0;
    trj->t2 = c->t0 + t1mt0 + t2mt1;
    trj->t3 = c->t0 + t3mt0;

    // Corresponding angle values with millicount/millideg precision
    int64_t mth0 = as_mcount(c->th0, c->th0_ext);
    int64_t mth1 = mth0 + x_time(trj->w0, t1mt0) + x_time2(trj->a0, t1mt0);
    int64_t mth2 = mth1 + x_time(trj->w1, t2mt1);
    int64_t mth3 = mth2 + x_time(trj->w1, t3mt2) + x_time2(trj->a2, t3mt2);

    // Store as counts and millicount
    as_count(mth0, &trj->th0, &trj->th0_ext);
    as_count(mth1, &trj->th1, &trj->th1_ext);
    as_count(mth2, &trj->th2, &trj->th2_ext);
    as_count(mth3, &trj->th3, &trj->th3_ext);
}

// Computes a trajectory for an angle command assuming *positive* speed
static void pbio_trajectory_new_forward_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // The given angle constraints won't change, so store them right away.
    trj->th0 = c->th0;
    trj->th3 = c->th3;
    trj->t0 = c->t0;

    // Store target speed.
    trj->w3 = c->w3;

    // Revisit: Drop ext and switch to increased angle resolution everywhere.
    trj->th0_ext = 0;
    trj->th1_ext = 0;
    trj->th2_ext = 0;
    trj->th3_ext = 0;

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is equal to or less than the target speed. Otherwise
    // it decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = c->w0 <= c->wt ? c->a0_abs : -c->a0_abs;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -c->a2_abs;

    // Check if we are going faster than the target speed already. If we are,
    // also check if slowing down still means we'd overshoot the angle target.
    if (trj->a0 < 0 && trj->w3 * trj->w3 - c->w0 * c->w0 >= 2 * trj->a2 * (trj->th3 - trj->th0)) {
        // We're going too fast to reach the target speed before overshooting
        // the angle target. So we cut down the initial speed to match.
        trj->w0 = pbio_math_sqrt(trj->w3 * trj->w3 - 2 * trj->a2 * (trj->th3 - trj->th0));

        // The initial speed is now cut down to make the out-phase just
        // feasible, so there is only that phase by definition. Everything else
        // has a zero duration and rotation.
        trj->th1 = trj->th0;
        trj->th2 = trj->th0;
        trj->t1 = trj->t0;
        trj->t2 = trj->t0;
        trj->w1 = trj->w0;

        // The final time is the end-time of the deceleration phase.
        trj->t3 = trj->t2 + wdiva(trj->w0 - trj->w3, trj->a2);

        // All trajectory parameters for the too-fast-decreasing case are now
        // set, so return the result.
        return;
    }

    // For all other cases, we can use the initial speed as-is.
    trj->w0 = c->w0;

    // Now we can evaluate the nominal cases and check if they hold. First get
    // a fictitious zero-speed angle for computational convenience. This is the
    // angle on the speed-angle phase plot where the in-phase square root
    // function intersects the zero speed axis if we kept (de)accelerating.
    int32_t thf = trj->th0 - trj->w0 * trj->w0 / (2 * trj->a0);

    // The in-phase completes at the point where it reaches the target speed.
    trj->th1 = thf + c->wt * c->wt / (2 * trj->a0);

    // The out-phase rotation is the angle traveled while slowing down. But if
    // the end speed equals the target speed, there is no out-phase.
    trj->th2 = trj->w3 == 0 ? trj->th3 + c->wt * c->wt / (2 * trj->a2): trj->th3;

    // In the decreasing case, we always reach the target speed, since the case
    // where it isn't reached is already handled above. For the increasing case
    // we still need to verify the feasibility. So as a starting point:
    trj->w1 = c->wt;

    // The aforementioned results are valid for the increasing case only if
    // the computed angle th2 is indeed larger than th1.
    if (trj->a0 > 0 && trj->th2 < trj->th1) {
        // The result is invalid. Then we will not reach the target speed, but
        // begin decelerating at the point where the in/out phases intersect.
        int32_t w1_sq = trj->w3 == 0 ?
            // When decelerating to zero, the insersection is as follows.
            // Zero division is avoided since a2 < 0 and a0 > 0 so the
            // denominator is strictly negative.
            2 * trj->a0 * trj->a2 * (trj->th3 - thf) / (trj->a2 - trj->a0) :
            // When skipping deceleration, it simplifies to:
            2 * trj->a0 * (trj->th3 - thf);

        // The peak speed is then the square root of aforementioned result
        // and the th1 and th2 angles are equal by definition.
        trj->w1 = pbio_math_sqrt(w1_sq);

        // The intermediate angle th1 depends on the end speed condition.
        if (trj->w3 == 0) {
            trj->th1 = thf + w1_sq / (2 * trj->a0);
        } else {
            trj->th1 = trj->th3;
            // The final speed is not reached either, so reaching
            // w1 is the best we can do in the given time.
            trj->w3 = trj->w1;
        }
        // There is no constant speed phase, so th2 = th1.
        trj->th2 = trj->th1;
    }

    // With the intermediate angles and speeds now known, we can calculate the
    // corresponding durations to match.
    trj->t1 = trj->t0 + wdiva(trj->w1 - trj->w0, trj->a0);
    trj->t2 = trj->t1 + wdiva(trj->th2 - trj->th1, trj->w1);
    trj->t3 = trj->t2 + wdiva(trj->w3 - trj->w1, trj->a2);
}

void pbio_trajectory_stretch(pbio_trajectory_t *trj, int32_t t1mt0, int32_t t2mt0, int32_t t3mt0) {

    if (t3mt0 == 0) {
        // This is a stationary maneuver, so there's nothing to recompute.
        return;
    }

    // This recomputes several components of a trajectory such that it travels
    // the same distance as before, but with new time stamps t1, t2, and t3.
    // Setting the speed integral equal to (th3 - th0) gives three constraint
    // equations with three unknowns (a0, a2, wt), for which we can solve.

    // Solve constraint to find peak velocity
    trj->w1 = (2 * (trj->th3 - trj->th0) - timest(trj->w0, t1mt0) - timest(trj->w3, t3mt0 - t2mt0)) * US_PER_MS /
        ((t3mt0 + t2mt0 - t1mt0) / US_PER_MS);

    // Get corresponding accelerations
    trj->a0 = t1mt0 == 0 ? 0 : (trj->w1 - trj->w0) * US_PER_MS / t1mt0 * MS_PER_SECOND;
    trj->a2 = (t3mt0 - t2mt0) == 0 ? 0 : (trj->w3 - trj->w1) * US_PER_MS / (t3mt0 - t2mt0) * MS_PER_SECOND;

    // Stretch time arguments
    trj->t1 = trj->t0 + t1mt0;
    trj->t2 = trj->t0 + t2mt0;
    trj->t3 = trj->t0 + t3mt0;

    // With all constraints already satisfied, we can just compute the
    // intermediate positions relative to the endpoints, given the now-known
    // accelerations and speeds.
    trj->th1 = trj->th0 + timest(trj->w0, t1mt0) + timest2(trj->a0, t1mt0);
    trj->th2 = trj->th1 + timest(trj->w1, t2mt0 - t1mt0);
}

static pbio_error_t pbio_trajectory_new_time_command(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {

    // Return error for negative or too long user-specified duration
    if (c->duration < 0 || c->duration / 1000 > DURATION_MAX_MS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Return empty maneuver for zero time
    if (c->duration == 0) {
        pbio_trajectory_make_constant(trj, c->t0, c->th0, 0);
        return PBIO_SUCCESS;
    }

    // Final speed can only be zero or equal to target speed.
    if (c->w3 != 0 && c->w3 != c->wt) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Remember if the original user-specified maneuver was backward.
    bool backward = c->wt < 0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c->wt *= -1;
        c->w0 *= -1;
        c->w3 *= -1;
    }

    // Bind target speed by maximum speed.
    c->wt = min(c->wt, c->wmax);

    // REVISIT: Send w3 same as bool flag.

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_time_command(trj, c);

    // Assert that all resulting time intervals are positive.
    if (trj->t1 - trj->t0 < 0 || trj->t2 - trj->t1 < 0 || trj->t3 - trj->t2 < 0) {
        return PBIO_ERROR_FAILED;
    }

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }
    return PBIO_SUCCESS;
}

static pbio_error_t pbio_trajectory_new_angle_command(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {

    // Return error for maneuver that is too long
    if (abs((c->th3 - c->th0) / c->wt) + 1 > DURATION_MAX_MS / 1000) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Return empty maneuver for zero angle or zero speed
    if (c->th3 == c->th0 || c->wt == 0) {
        pbio_trajectory_make_constant(trj, c->t0, c->th0, 0);
        return PBIO_SUCCESS;
    }

    // Final speed can only be zero or equal to target speed.
    if (c->w3 != 0 && c->w3 != c->wt) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Direction is solely defined in terms of th3 position relative to th0.
    // For speed, only the *magnitude* is relevant. Certain end-user APIs
    // allow specifying phyically impossible scenarios like negative speed
    // with a positive relative position. Those cases are not handled here and
    // should be appropriately handled at higher levels.
    c->wt = abs(c->wt);
    c->w3 = abs(c->w3);

    // Bind target speed by maximum speed.
    c->wt = min(c->wt, c->wmax);

    // Check if the original user-specified maneuver is backward.
    bool backward = c->th3 < c->th0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c->th3 = 2 * c->th0 - c->th3;
        c->w0 *= -1;
    }

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_angle_command(trj, c);

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_trajectory_calculate_new(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {
    // Handle trajectory with angle end point constraint, and we calculate the unknown end time.
    if (c->type == PBIO_TRAJECTORY_TYPE_ANGLE) {
        return pbio_trajectory_new_angle_command(trj, c);
    }
    // Otherwise, the end point is in time. We calculate the unknown end angle.
    return pbio_trajectory_new_time_command(trj, c);
}

pbio_error_t pbio_trajectory_extend(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {

    // Get current reference point and acceleration, which will be the 0-point for the new trajectory.
    pbio_trajectory_reference_t ref;
    pbio_trajectory_get_reference(trj, c->t0, &ref);

    c->th0 = ref.count;
    c->th0_ext = ref.count_ext;
    c->w0 = ref.rate;
    int32_t acceleration_ref = ref.acceleration;

    // First get the nominal commanded trajectory. This will be our default if we can't patch onto the existing one.
    pbio_error_t err;
    pbio_trajectory_t nominal;
    if (c->type == PBIO_TRAJECTORY_TYPE_ANGLE) {
        err = pbio_trajectory_new_angle_command(&nominal, c);
    } else {
        err = pbio_trajectory_new_time_command(&nominal, c);
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
        if (c->t0 - trj->t1 < 0) {
            // We are still in the acceleration segment, so we can restart from its starting point
            c->t0 = trj->t0;
            c->w0 = trj->w0;
            c->th0 = trj->th0;
            c->th0_ext = trj->th0_ext;
        } else if (c->t0 - trj->t2 < 0) {
            // We are in the constant speed phase, so we can restart from its starting point
            c->t0 = trj->t1;
            c->w0 = trj->w1;
            c->th0 = trj->th1;
            c->th0_ext = trj->th1_ext;
        } else if (c->t0 - trj->t3 < 0) {
            // We are in the deceleration phase, so we can restart from its starting point
            c->t0 = trj->t2;
            c->w0 = trj->w1;
            c->th0 = trj->th2;
            c->th0_ext = trj->th2_ext;
        } else {
            // We are in the zero speed phase, so we can restart from its starting point
            c->t0 = trj->t3;
            c->w0 = 0;
            c->th0 = trj->th3;
            c->th0_ext = trj->th3_ext;
        }

        // We shifted the start time into the past, so we must adjust duration accordingly.
        c->duration += (nominal.t0 - c->t0);

        // Now we can make the new trajectory with a starting point coincident
        // with a point on the existing trajectory
        if (c->type == PBIO_TRAJECTORY_TYPE_ANGLE) {
            return pbio_trajectory_new_angle_command(trj, c);
        } else {
            return pbio_trajectory_new_time_command(trj, c);
        }

    } else {
        // Trajectories were not tangent, so just return the nominal, unpatched trajectory
        *trj = nominal;
        return PBIO_SUCCESS;
    }
}


// Evaluate the reference speed and velocity at the (shifted) time
void pbio_trajectory_get_reference(pbio_trajectory_t *trj, int32_t time_ref, pbio_trajectory_reference_t *ref) {

    int64_t mcount_ref;

    if (time_ref - trj->t1 < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        ref->rate = trj->w0 + timest(trj->a0, time_ref - trj->t0);
        mcount_ref = as_mcount(trj->th0, trj->th0_ext) + x_time(trj->w0, time_ref - trj->t0) + x_time2(trj->a0, time_ref - trj->t0);
        ref->acceleration = trj->a0;
    } else if (time_ref - trj->t2 <= 0) {
        // If we are here, then we are in the constant speed phase
        ref->rate = trj->w1;
        mcount_ref = as_mcount(trj->th1, trj->th1_ext) + x_time(trj->w1, time_ref - trj->t1);
        ref->acceleration = 0;
    } else if (time_ref - trj->t3 <= 0) {
        // If we are here, then we are in the deceleration phase
        ref->rate = trj->w1 + timest(trj->a2, time_ref - trj->t2);
        mcount_ref = as_mcount(trj->th2, trj->th2_ext) + x_time(trj->w1, time_ref - trj->t2) + x_time2(trj->a2, time_ref - trj->t2);
        ref->acceleration = trj->a2;
    } else {
        // If we are here, we are in the constant speed phase after the maneuver completes
        ref->rate = trj->w3;
        mcount_ref = as_mcount(trj->th3, trj->th3_ext) + x_time(trj->w3, time_ref - trj->t3);
        ref->acceleration = 0;

        // To avoid any overflows of the aforementioned time comparisons,
        // rebase the trajectory if it has been running a long time.
        if (time_ref - trj->t0 > DURATION_MAX_MS * US_PER_MS) {
            // REVISIT: update with new ext value or implement resolution fix.
            as_count(mcount_ref, &ref->count, &ref->count_ext);
            pbio_trajectory_make_constant(trj, time_ref, ref->count, trj->w3);
            trj->th3_ext = ref->count_ext;
        }
    }

    // Split high res angle into counts and millicounts
    as_count(mcount_ref, &ref->count, &ref->count_ext);

}
