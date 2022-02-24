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

void pbio_trajectory_make_stationary(pbio_trajectory_t *trj, int32_t t0, int32_t th0) {
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

    // This is a finite maneuver
    trj->forever = false;
}

static int64_t x_time(int32_t b, int32_t t) {
    return (((int64_t)b) * ((int64_t)t)) / US_PER_MS;
}

static int64_t x_time2(int32_t b, int32_t t) {
    return x_time(x_time(b, t), t) / (2 * US_PER_MS);
}

static pbio_error_t pbio_trajectory_new_time_command(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt0;
    int32_t t3mt2;
    int32_t t2mt1;
    int32_t t1mt0;

    // Duration of the maneuver
    if (c->duration == DURATION_FOREVER) {
        // In case of forever, we set the duration to a fictitious 60 seconds.
        t3mt0 = 60 * US_PER_SECOND;
        // This is an infinite maneuver. (This means we'll just ignore the deceleration
        // phase when computing references later, so we keep going even after 60 seconds.)
        trj->forever = true;
    } else {
        // Otherwise, the interval is just the duration
        t3mt0 = c->duration;
        // This is a finite maneuver
        trj->forever = false;
    }

    // Return error for negative user-specified duration
    if (t3mt0 < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = c->wt < 0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        c->wt *= -1;
        c->w0 *= -1;
    }

    // Limit initial speed
    int32_t max_init = timest(c->a0_abs, t3mt0);
    int32_t abs_max = min(c->wmax, max_init);
    c->w0 = pbio_math_clamp(c->w0, abs_max);
    c->wt = pbio_math_clamp(c->wt, abs_max);

    // Initial speed is less than the target speed
    if (c->w0 < c->wt) {
        // Therefore accelerate
        trj->a0 = c->a0_abs;
        // If target speed can be reached
        if (wdiva(c->wt - c->w0, c->a0_abs) - (t3mt0 - wdiva(c->w0, c->a0_abs)) / 2 < 0) {
            t1mt0 = wdiva(c->wt - c->w0, c->a0_abs);
            trj->w1 = c->wt;
        }
        // If target speed cannot be reached
        else {
            t1mt0 = (t3mt0 - wdiva(c->w0, c->a0_abs)) / 2;
            trj->w1 = timest(c->a0_abs, t3mt0) / 2 + c->w0 / 2;
        }
    }
    // Initial speed is more than the target speed
    else if (c->w0 > c->wt) {
        // Therefore decelerate
        trj->a0 = -c->a0_abs;
        t1mt0 = wdiva(c->w0 - c->wt, c->a0_abs);
        trj->w1 = c->wt;
    }
    // Initial speed is equal to the target speed
    else {
        // Therefore no acceleration
        trj->a0 = 0;
        t1mt0 = 0;
        trj->w1 = c->wt;
    }

    // # Deceleration phase
    trj->a2 = -c->a0_abs;
    t3mt2 = wdiva(trj->w1, c->a0_abs);

    // Constant speed duration
    t2mt1 = t3mt0 - t3mt2 - t1mt0;

    // Assert that all time intervals are positive
    if (t1mt0 < 0 || t2mt1 < 0 || t3mt2 < 0) {
        return PBIO_ERROR_FAILED;
    }

    // Store other results/arguments
    trj->w0 = c->w0;
    trj->t0 = c->t0;
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

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(trj);
    }

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_trajectory_new_angle_command(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {

    // Return error for maneuver that is too long
    if (abs((c->th3 - c->th0) / c->wt) + 1 > DURATION_MAX_S) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Return empty maneuver for zero angle or zero speed
    if (c->th3 == c->th0 || c->wt == 0) {
        pbio_trajectory_make_stationary(trj, c->t0, c->th0);
        return PBIO_SUCCESS;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = c->th3 < c->th0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        c->th3 = 2 * c->th0 - c->th3;
        c->w0 *= -1;
    }

    // In a forward maneuver, the target speed is always positive.
    c->wt = abs(c->wt);
    c->wt = min(c->wt, c->wmax);

    // Limit initial speed
    c->w0 = pbio_math_clamp(c->w0, c->wmax);

    // Limit initial speed, but evaluate square root only if necessary (usually not)
    if (c->w0 > 0 && (c->w0 * c->w0) / (2 * c->a0_abs) > c->th3 - c->th0) {
        c->w0 = pbio_math_sqrt(2 * c->a0_abs * (c->th3 - c->th0));

        // In this situation, speed is just a linearly descending line
        // from the (capped) starting speed towards zero. Hence, t2=t1 which is
        // anywhere between t3 and t0. We have to pick something, so set it to
        // the way point.
        c->wt = c->w0 / 2;
    }

    // Initial speed is less than the target speed
    if (c->w0 < c->wt) {
        // Therefore accelerate towards intersection from below,
        // either by reaching constant speed phase or not.
        trj->a0 = c->a0_abs;

        // Fictitious zero speed angle (ahead of us if we have negative initial speed; behind us if we have initial positive speed)
        int32_t thf = c->th0 - (c->w0 * c->w0) / (2 * c->a0_abs);

        // Test if we can get to trj speed
        if (c->th3 - thf >= (c->wt * c->wt) / c->a0_abs) {
            //  If so, find both constant speed intersections
            trj->th1 = thf + (c->wt * c->wt) / (2 * c->a0_abs);
            trj->th2 = c->th3 - (c->wt * c->wt) / (2 * c->a0_abs);
            trj->w1 = c->wt;
        } else {
            // Otherwise, intersect halfway between accelerating and decelerating square root arcs
            trj->th1 = (c->th3 + thf) / 2;
            trj->th2 = trj->th1;
            trj->w1 = pbio_math_sqrt(2 * c->a0_abs * (trj->th1 - thf));
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate towards intersection from above
        trj->a0 = -c->a0_abs;
        trj->th1 = c->th0 + (c->w0 * c->w0 - c->wt * c->wt) / (2 * c->a0_abs);
        trj->th2 = c->th3 - (c->wt * c->wt) / (2 * c->a0_abs);
        trj->w1 = c->wt;
    }
    // Corresponding time intervals
    int32_t t1mt0 = wdiva(trj->w1 - c->w0, trj->a0);
    int32_t t2mt1 = trj->th2 == trj->th1 ? 0 : wdiva(trj->th2 - trj->th1, trj->w1);
    int32_t t3mt2 = wdiva(trj->w1, c->a0_abs);

    // Store other results/arguments
    trj->w0 = c->w0;
    trj->th0 = c->th0;
    trj->th3 = c->th3;
    trj->t0 = c->t0;
    trj->t1 = c->t0 + t1mt0;
    trj->t2 = trj->t1 + t2mt1;
    trj->t3 = trj->t2 + t3mt2;
    trj->a2 = -c->a0_abs;

    // FIXME: Angle based does not have high res yet
    trj->th0_ext = 0;
    trj->th1_ext = 0;
    trj->th2_ext = 0;
    trj->th3_ext = 0;

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(trj);
    }

    // This is a finite maneuver
    trj->forever = false;

    return PBIO_SUCCESS;
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

    // Solve constraint to find initial acceleration
    trj->a0 = ((int64_t)(trj->th3 - trj->th0) * US_PER_SECOND * 2 - (int64_t)trj->w0 * (t3mt0 + t2mt0)) /
        t1mt0 * US_PER_SECOND / (t3mt0 + t2mt0 - t1mt0);

    // Get corresponding target speed and final deceleration
    trj->w1 = timest(trj->a0, t1mt0) + trj->w0;
    trj->a2 = (int64_t)trj->w1 * US_PER_SECOND / (t2mt0 - t3mt0);

    // Stretch time arguments
    trj->t1 = trj->t0 + t1mt0;
    trj->t2 = trj->t0 + t2mt0;
    trj->t3 = trj->t0 + t3mt0;

    // With all constraints already satisfied, we can just compute the
    // intermediate positions relative to the endpoints, given the now-known
    // accelerations and speeds.
    trj->th1 = trj->th0 + (trj->w1 * trj->w1 - trj->w0 * trj->w0) / (2 * trj->a0);
    trj->th2 = trj->th3 + (trj->w1 * trj->w1) / (2 * trj->a2);
}


pbio_error_t pbio_trajectory_calculate_new(pbio_trajectory_t *trj, pbio_trajectory_command_t *c) {
    // Handle trajectory with angle end point constraint, and we calculate the unknown end time.
    if (c->type == PBIO_TRAJECTORY_TYPE_ANGLE) {
        return pbio_trajectory_new_angle_command(trj, c);
    }
    // Otherwise, the end point is in time, or forever (which is just a long time). We calculate the unknown end angle.
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
        } else if (trj->forever || c->t0 - trj->t2 < 0) {
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

        // We shifted the start time into the past, so we must adjust duration accordingly. But forever remains forever.
        if (c->type != PBIO_TRAJECTORY_TYPE_FOREVER) {
            c->duration += (nominal.t0 - c->t0);
        }

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
    } else if (trj->forever || time_ref - trj->t2 <= 0) {
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
        // If we are here, we are in the zero speed phase (relevant when holding position)
        ref->rate = 0;
        mcount_ref = as_mcount(trj->th3, trj->th3_ext);
        ref->acceleration = 0;
    }

    // Split high res angle into counts and millicounts
    as_count(mcount_ref, &ref->count, &ref->count_ext);

    // Rebase the reference before it overflows after 35 minutes
    if (time_ref - trj->t0 > (DURATION_MAX_S + 120) * MS_PER_SECOND * US_PER_MS) {
        // Infinite maneuvers just maintain the same reference speed, continuing again from current time
        if (trj->forever) {

            // REVISIT: Once the final speed is settable, we no longer need a special case for forever anywhere.

            pbio_trajectory_command_t command = {
                .type = PBIO_TRAJECTORY_TYPE_FOREVER,
                .t0 = time_ref,
                .th0 = ref->count,
                .th0_ext = ref->count_ext,
                .duration = (int32_t) DURATION_FOREVER,
                .w0 = trj->w1,
                .wt = trj->w1,
                .wmax = trj->w1,
                .a0_abs = abs(trj->a2),
                .a2_abs = abs(trj->a2),
            };

            pbio_trajectory_new_time_command(trj, &command);
        }
        // All other maneuvers are considered complete and just stop. In practice, other maneuvers are not
        // allowed to be this long. This just ensures that if a motor stops and holds, it will continue to
        // do so forever, by rebasing the stationary trajectory before it overflows.
        else {
            pbio_trajectory_make_stationary(trj, time_ref, ref->count);
        }

    }
}
