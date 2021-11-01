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

pbio_error_t pbio_trajectory_calc_angle_new(pbio_trajectory_t *trj, int32_t t0, int32_t duration, int32_t th0, int32_t th0_ext, int32_t w0, int32_t wt, int32_t wmax, int32_t a) {

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt0;
    int32_t t3mt2;
    int32_t t2mt1;
    int32_t t1mt0;

    // Duration of the maneuver
    if (duration == DURATION_FOREVER) {
        // In case of forever, we set the duration to a fictitious 60 seconds.
        t3mt0 = 60 * US_PER_SECOND;
        // This is an infinite maneuver. (This means we'll just ignore the deceleration
        // phase when computing references later, so we keep going even after 60 seconds.)
        trj->forever = true;
    } else {
        // Otherwise, the interval is just the duration
        t3mt0 = duration;
        // This is a finite maneuver
        trj->forever = false;
    }

    // Return error for negative user-specified duration
    if (t3mt0 < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = wt < 0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        wt *= -1;
        w0 *= -1;
    }

    // Limit initial speed
    int32_t max_init = timest(a, t3mt0);
    int32_t abs_max = min(wmax, max_init);
    w0 = max(-abs_max, min(w0, abs_max));
    wt = max(-abs_max, min(wt, abs_max));

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate
        trj->a0 = a;
        // If target speed can be reached
        if (wdiva(wt - w0, a) - (t3mt0 - wdiva(w0, a)) / 2 < 0) {
            t1mt0 = wdiva(wt - w0, a);
            trj->w1 = wt;
        }
        // If target speed cannot be reached
        else {
            t1mt0 = (t3mt0 - wdiva(w0, a)) / 2;
            trj->w1 = timest(a, t3mt0) / 2 + w0 / 2;
        }
    }
    // Initial speed is more than the target speed
    else if (w0 > wt) {
        // Therefore decelerate
        trj->a0 = -a;
        t1mt0 = wdiva(w0 - wt, a);
        trj->w1 = wt;
    }
    // Initial speed is equal to the target speed
    else {
        // Therefore no acceleration
        trj->a0 = 0;
        t1mt0 = 0;
        trj->w1 = wt;
    }

    // # Deceleration phase
    trj->a2 = -a;
    t3mt2 = wdiva(trj->w1, a);

    // Constant speed duration
    t2mt1 = t3mt0 - t3mt2 - t1mt0;

    // Assert that all time intervals are positive
    if (t1mt0 < 0 || t2mt1 < 0 || t3mt2 < 0) {
        return PBIO_ERROR_FAILED;
    }

    // Store other results/arguments
    trj->w0 = w0;
    trj->t0 = t0;
    trj->t1 = t0 + t1mt0;
    trj->t2 = t0 + t1mt0 + t2mt1;
    trj->t3 = t0 + t3mt0;

    // Corresponding angle values with millicount/millideg precision
    int64_t mth0 = as_mcount(th0, th0_ext);
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

pbio_error_t pbio_trajectory_calc_time_new(pbio_trajectory_t *trj, int32_t t0, int32_t th0, int32_t th3, int32_t w0, int32_t wt, int32_t wmax, int32_t a) {

    // Return error for zero speed
    if (wt == 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Return error for maneuver that is too long
    if (abs((th3 - th0) / wt) + 1 > DURATION_MAX_S) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Return empty maneuver for zero angle
    if (th3 == th0) {
        pbio_trajectory_make_stationary(trj, t0, th0);
        return PBIO_SUCCESS;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = th3 < th0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        th3 = 2 * th0 - th3;
        w0 *= -1;
    }

    // In a forward maneuver, the target speed is always positive.
    wt = abs(wt);
    wt = min(wt, wmax);

    // Limit initial speed
    w0 = max(-wmax, min(w0, wmax));

    // Limit initial speed, but evaluate square root only if necessary (usually not)
    if (w0 > 0 && (w0 * w0) / (2 * a) > th3 - th0) {
        w0 = pbio_math_sqrt(2 * a * (th3 - th0));
    }

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate towards intersection from below,
        // either by reaching constant speed phase or not.
        trj->a0 = a;

        // Fictitious zero speed angle (ahead of us if we have negative initial speed; behind us if we have initial positive speed)
        int32_t thf = th0 - (w0 * w0) / (2 * a);

        // Test if we can get to trj speed
        if (th3 - thf >= (wt * wt) / a) {
            //  If so, find both constant speed intersections
            trj->th1 = thf + (wt * wt) / (2 * a);
            trj->th2 = th3 - (wt * wt) / (2 * a);
            trj->w1 = wt;
        } else {
            // Otherwise, intersect halfway between accelerating and decelerating square root arcs
            trj->th1 = (th3 + thf) / 2;
            trj->th2 = trj->th1;
            trj->w1 = pbio_math_sqrt(2 * a * (trj->th1 - thf));
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate towards intersection from above
        trj->a0 = -a;
        trj->th1 = th0 + (w0 * w0 - wt * wt) / (2 * a);
        trj->th2 = th3 - (wt * wt) / (2 * a);
        trj->w1 = wt;
    }
    // Corresponding time intervals
    int32_t t1mt0 = wdiva(trj->w1 - w0, trj->a0);
    int32_t t2mt1 = trj->th2 == trj->th1 ? 0 : wdiva(trj->th2 - trj->th1, trj->w1);
    int32_t t3mt2 = wdiva(trj->w1, a);

    // Store other results/arguments
    trj->w0 = w0;
    trj->th0 = th0;
    trj->th3 = th3;
    trj->t0 = t0;
    trj->t1 = t0 + t1mt0;
    trj->t2 = trj->t1 + t2mt1;
    trj->t3 = trj->t2 + t3mt2;
    trj->a2 = -a;

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
            pbio_trajectory_calc_angle_new(trj, time_ref, DURATION_FOREVER, ref->count, ref->count_ext, trj->w1, trj->w1, trj->w1, abs(trj->a2));
        }
        // All other maneuvers are considered complete and just stop. In practice, other maneuvers are not
        // allowed to be this long. This just ensures that if a motor stops and holds, it will continue to
        // do so forever, by rebasing the stationary trajectory before it overflows.
        else {
            pbio_trajectory_make_stationary(trj, time_ref, ref->count);
        }

    }
}
