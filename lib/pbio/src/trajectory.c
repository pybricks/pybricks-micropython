// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <assert.h>
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
    trj->w3 *= -1;
    trj->a0 *= -1;
    trj->a2 *= -1;
}

void pbio_trajectory_make_constant(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {
    // All times equal to initial time:
    trj->t0 = c->t0;
    trj->t1 = c->t0;
    trj->t2 = c->t0;
    trj->t3 = c->t0;

    // All angles equal to initial angle:
    trj->th0 = c->th0;
    trj->th1 = c->th0;
    trj->th2 = c->th0;
    trj->th3 = c->th0;
    trj->th0_ext = c->th0_ext;
    trj->th1_ext = c->th0_ext;
    trj->th2_ext = c->th0_ext;
    trj->th3_ext = c->th0_ext;

    // All accelerations zero:
    trj->a0 = 0;
    trj->a2 = 0;

    // Set speeds:
    trj->w0 = c->wt;
    trj->w1 = c->wt;
    trj->w3 = c->continue_running ? c->wt : 0;
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

    // Speed continues at target speed or goes to zero.
    trj->w3 = c->continue_running ? c->wt : 0;

    // Bind initial speed to make solution feasible.
    trj->w0 = c->w0;
    if (trj->w0 * US_PER_SECOND / c->duration < -c->a0_abs) {
        trj->w0 = -timest(c->a0_abs, c->duration);
    }
    if ((trj->w0 - trj->w3) * US_PER_SECOND / c->duration > max(c->a0_abs, c->a2_abs)) {
        trj->w0 = trj->w3 + timest(max(c->a0_abs, c->a2_abs), c->duration);
    }

    // Bind target speed to make solution feasible.
    int32_t wt = c->wt;
    if ((wt - trj->w3) * US_PER_SECOND / c->duration > c->a2_abs) {
        wt = trj->w3 + timest(c->a2_abs, c->duration);
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? c->a0_abs : -c->a0_abs;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -c->a2_abs;

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt0 = c->duration;
    int32_t t3mt2;
    int32_t t2mt1;
    int32_t t1mt0;

    // The in-phase completes at the point where it reaches the target speed.
    t1mt0 = wdiva(wt - trj->w0, trj->a0);

    // The out-phase rotation is the time passed while slowing down.
    t3mt2 = wdiva(trj->w3 - wt, trj->a2);

    // Get the time and speed for the constant speed phase.
    t2mt1 = t3mt0 - t1mt0 - t3mt2;
    trj->w1 = wt;

    // The aforementioned results are valid only if the computed time intervals
    // don't overlap, so t2mt1 must be positive.
    if (t2mt1 < 0) {
        // The result is invalid. Then we will not reach the target speed, but
        // begin decelerating at the point where the in/out phases intersect.
        // The valid result depends on whether there is an end speed or not.

        if (c->continue_running && trj->a0 > 0) {
            // If we have a nonzero final speed and initial positive
            // acceleration, the result can only be invalid if there is not
            // enough time to reach the final speed. Then, we just accelerate
            // the best we can in the available time.
            t1mt0 = t3mt0;
            t2mt1 = 0;
            t3mt2 = 0;
            trj->w1 = trj->w0 + timest(trj->a0, t3mt0);
            trj->w3 = trj->w1;
        } else {
            // Otherwise, we can just take the intersection of the accelerating
            // and decelerating ramps to find the speed at t1 = t2.
            assert(trj->a0 != trj->a2);
            t1mt0 = wdiva(trj->w3 - trj->w0 - timest(trj->a2, t3mt0), trj->a0 - trj->a2);

            // There is no constant speed phase in this case.
            t2mt1 = 0;
            t3mt2 = t3mt0 - t1mt0;
            trj->w1 = trj->w0 + timest(trj->a0, t1mt0);
        }
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

    // Speed continues at target speed or goes to zero.
    trj->w3 = c->continue_running ? c->wt : 0;

    // Revisit: Drop ext and switch to increased angle resolution everywhere.
    trj->th0_ext = 0;
    trj->th1_ext = 0;
    trj->th2_ext = 0;
    trj->th3_ext = 0;

    // Bind initial speed to make solution feasible. Do the larger-than check
    // using quadratic terms to avoid square root evaluations in most cases.
    // This is only needed for positive initial speed, because negative initial
    // speeds are always feasible in angle-based maneuvers.
    int32_t a_max = max(c->a0_abs, c->a2_abs);
    trj->w0 = c->w0;
    if (trj->w0 > 0 && trj->w0 * trj->w0 - trj->w3 * trj->w3 > 2 * a_max * (trj->th3 - trj->th0)) {
        trj->w0 = pbio_math_sqrt(trj->w3 * trj->w3 + 2 * a_max * (trj->th3 - trj->th0));
    }

    // Do the same check for the target speed, but now use the deceleration
    // magnitude as the only constraint. The target speed is always positive,
    // so we can omit that additional check here.
    int32_t wt = c->wt;
    if (wt * wt - trj->w3 * trj->w3 > 2 * c->a2_abs * (trj->th3 - trj->th0)) {
        wt = pbio_math_sqrt(trj->w3 * trj->w3 + 2 * c->a2_abs * (trj->th3 - trj->th0));
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? c->a0_abs : -c->a0_abs;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -c->a2_abs;

    // Now we can evaluate the nominal cases and check if they hold. First get
    // a fictitious zero-speed angle for computational convenience. This is the
    // angle on the speed-angle phase plot where the in-phase square root
    // function intersects the zero speed axis if we kept (de)accelerating.
    int32_t thf = trj->th0 - trj->w0 * trj->w0 / (2 * trj->a0);

    // The in-phase completes at the point where it reaches the target speed.
    trj->th1 = thf + wt * wt / (2 * trj->a0);

    // The out-phase rotation is the angle traveled while slowing down. But if
    // the end speed equals the target speed, there is no out-phase.
    trj->th2 = trj->th3 + (wt * wt - trj->w3 * trj->w3) / (2 * trj->a2);

    // In the nominal case, we always reach the target speed, so start with:
    trj->w1 = wt;

    // The aforementioned results are valid only if
    // the computed angle th2 is indeed larger than th1.
    if (trj->th2 < trj->th1) {
        // The result is invalid. Then we will not reach the target speed, but
        // begin decelerating at the point where the in/out phases intersect.

        if (c->continue_running && trj->a0 > 0) {
            // If we have a nonzero final speed and initial positive
            // acceleration, the result can only be invalid if there is not
            // enough time to reach the final speed. Then, we just accelerate
            // the best we can in the available rotation.
            trj->w1 = pbio_math_sqrt(2 * trj->a0 * (trj->th3 - thf));
            trj->th1 = trj->th3;
            trj->th2 = trj->th1;

            // The final speed is not reached either, so reaching
            // w1 is the best we can do in the given time.
            trj->w3 = trj->w1;
        } else {
            // Otherwise, we can just take the intersection of the accelerating
            // and decelerating ramps to find the speed at t1 = t2.
            int32_t w1_squared = 2 * trj->a0 * trj->a2 * (trj->th3 - thf) / (trj->a2 - trj->a0);
            trj->w1 = pbio_math_sqrt(w1_squared);
            trj->th1 = thf + w1_squared / (2 * trj->a0);
            trj->th2 = trj->th1;
        }
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

pbio_error_t pbio_trajectory_new_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command) {

    // Copy the command so we can modify it.
    pbio_trajectory_command_t c = *command;

    // Return error for negative or too long user-specified duration
    if (c.duration < 0 || c.duration / 1000 > DURATION_MAX_MS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Return empty maneuver for zero time
    if (c.duration == 0) {
        c.wt = 0;
        c.continue_running = false;
        pbio_trajectory_make_constant(trj, &c);
        return PBIO_SUCCESS;
    }

    // Remember if the original user-specified maneuver was backward.
    bool backward = c.wt < 0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c.wt *= -1;
        c.w0 *= -1;
    }

    // Bind target speed by maximum speed.
    c.wt = min(c.wt, c.wmax);

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_time_command(trj, &c);

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

pbio_error_t pbio_trajectory_new_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command) {

    // Copy the command so we can modify it.
    pbio_trajectory_command_t c = *command;

    // Return error for maneuver that is too long
    if (abs((c.th3 - c.th0) / c.wt) + 1 > DURATION_MAX_MS / 1000) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Return empty maneuver for zero angle or zero speed
    if (c.th3 == c.th0 || c.wt == 0) {
        c.wt = 0;
        c.continue_running = false;
        pbio_trajectory_make_constant(trj, &c);
        return PBIO_SUCCESS;
    }

    // Direction is solely defined in terms of th3 position relative to th0.
    // For speed, only the *magnitude* is relevant. Certain end-user APIs
    // allow specifying phyically impossible scenarios like negative speed
    // with a positive relative position. Those cases are not handled here and
    // should be appropriately handled at higher levels.
    c.wt = abs(c.wt);

    // Bind target speed by maximum speed.
    c.wt = min(c.wt, c.wmax);

    // Check if the original user-specified maneuver is backward.
    bool backward = c.th3 < c.th0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c.th3 = 2 * c.th0 - c.th3;
        c.w0 *= -1;
    }

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_angle_command(trj, &c);

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }

    return PBIO_SUCCESS;
}

void pbio_trajectory_get_last_vertex(pbio_trajectory_t *trj, int32_t time_ref, int32_t *time, pbio_trajectory_reference_t *ref) {
    // Find which section of the ongoing maneuver we were in, and take
    // corresponding segment starting point.
    if (time_ref - trj->t1 < 0) {
        // Acceleration segment.
        *time = trj->t0;
        ref->rate = trj->w0;
        ref->count = trj->th0;
        ref->count_ext = trj->th0_ext;
    } else if (time_ref - trj->t2 < 0) {
        // Constant speed segment.
        *time = trj->t1;
        ref->rate = trj->w1;
        ref->count = trj->th1;
        ref->count_ext = trj->th1_ext;
    } else if (time_ref - trj->t3 < 0) {
        // Deceleration segment.
        *time = trj->t2;
        ref->rate = trj->w1;
        ref->count = trj->th2;
        ref->count_ext = trj->th2_ext;
    } else {
        // Final speed segment.
        *time = trj->t3;
        ref->rate = trj->w3;
        ref->count = trj->th3;
        ref->count_ext = trj->th3_ext;
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
        if (time_ref - trj->t0 > DURATION_FOREVER_MS * US_PER_MS) {
            pbio_trajectory_command_t command = {
                .t0 = time_ref,
                .wt = trj->w3,
                .continue_running = true,
            };
            as_count(mcount_ref, &command.th0, &command.th0_ext);
            pbio_trajectory_make_constant(trj, &command);
        }
    }

    // Split high res angle into counts and millicounts
    as_count(mcount_ref, &ref->count, &ref->count_ext);

}
