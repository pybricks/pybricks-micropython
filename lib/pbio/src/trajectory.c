// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <contiki.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>

void reverse_trajectory(pbio_trajectory_t *ref) {
    // Mirror angles about initial angle th0
    ref->th1 = 2*ref->th0 - ref->th1;
    ref->th2 = 2*ref->th0 - ref->th2;
    ref->th3 = 2*ref->th0 - ref->th3;
    ref->mth1 = 2*ref->mth0 - ref->mth1;
    ref->mth2 = 2*ref->mth0 - ref->mth2;
    ref->mth3 = 2*ref->mth0 - ref->mth3;

    // Negate speeds and accelerations
    ref->w0 *= -1;
    ref->w1 *= -1;
    ref->a0 *= -1;
    ref->a2 *= -1;
}

void pbio_trajectory_make_stationary(pbio_trajectory_t *ref, int32_t t0, int32_t th0, int32_t w1) {
    // All times equal to initial time:
    ref->t0 = t0;
    ref->t1 = t0;
    ref->t2 = t0;
    ref->t3 = t0;

    // All angles equal to initial angle:
    ref->th0 = th0;
    ref->th1 = th0;
    ref->th2 = th0;
    ref->th3 = th0;
    ref->mth0 = to_mcount(th0);
    ref->mth1 = to_mcount(th0);
    ref->mth2 = to_mcount(th0);
    ref->mth3 = to_mcount(th0);

    // All speeds/accelerations zero:
    ref->w0 = 0;
    ref->w1 = w1;
    ref->a0 = 0;
    ref->a2 = 0;

    // This is a finite maneuver
    ref->forever = false;
}

static int64_t x_time(int32_t b, int32_t t) {
    return (((int64_t) b) * ((int64_t) t))/US_PER_MS;
}

static int64_t x_time2(int32_t b, int32_t t) {
    return x_time(x_time(b, t), t)/(2*US_PER_MS);
}

pbio_error_t pbio_trajectory_make_time_based(pbio_trajectory_t *ref, bool forever, int32_t t0, int32_t t3, int64_t mth0, int32_t w0, int32_t wt, int32_t wmax, int32_t a) {

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt0;
    int32_t t3mt2;
    int32_t t2mt1;
    int32_t t1mt0;

    // Duration of the maneuver
    if (forever) {
        // In case of forever, we set the duration to a fictitious 60 seconds.
        t3mt0 = 60*US_PER_SECOND;
        // This is an infinite maneuver. (This means we'll just ignore the deceleration
        // phase when computing references later, so we keep going even after 60 seconds.)
        ref->forever = true; 
    }
    else {
        // Otherwise, the duration is just the end time minus start time
        t3mt0 = t3-t0;
        // This is a finite maneuver
        ref->forever = false;
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

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate
        ref->a0 = a;
        // If target speed can be reached
        if (wdiva(wt-w0, a) - (t3mt0-wdiva(w0, a))/2 < 0) {
            t1mt0 = wdiva(wt-w0, a);
            ref->w1 = wt;
        }
        // If target speed cannot be reached
        else {
            t1mt0 = (t3mt0-wdiva(w0, a))/2;
            ref->w1 = timest(a, t3mt0)/2+w0/2;
        }
    }
    // Initial speed is more than the target speed
    else if (w0 > wt) {
        // Therefore decelerate
        ref->a0 = -a;
        t1mt0 = wdiva(w0-wt, a);
        ref->w1 = wt;
    }
    // Initial speed is equal to the target speed
    else {
        // Therefore no acceleration
        ref->a0 = 0;
        t1mt0 = 0;
        ref->w1 = wt;
    }

    // # Deceleration phase
    ref->a2 = -a;
    t3mt2 = wdiva(ref->w1, a);

    // Constant speed duration
    t2mt1 = t3mt0 - t3mt2 - t1mt0;

    // Assert that all time intervals are positive
    if (t1mt0 < 0 || t2mt1 < 0 || t3mt2 < 0) {
        return PBIO_ERROR_FAILED;
    }

    // Store other results/arguments
    ref->w0 = w0;
    ref->t0 = t0;
    ref->t1 = t0 + t1mt0;
    ref->t2 = t0 + t1mt0 + t2mt1;
    ref->t3 = t3;

    // Corresponding angle values with millicount/millideg precision
    ref->mth0 = mth0;
    ref->mth1 = ref->mth0 + x_time(ref->w0, t1mt0) + x_time2(ref->a0, t1mt0);
    ref->mth2 = ref->mth1 + x_time(ref->w1, t2mt1);
    ref->mth3 = ref->mth2 + x_time(ref->w1, t3mt2) + x_time2(ref->a2, t3mt2);

    // Keep regular count values for code that does not need mdeg precision
    ref->th0 = to_count(ref->mth0);
    ref->th1 = to_count(ref->mth1);
    ref->th2 = to_count(ref->mth2);
    ref->th3 = to_count(ref->mth3);

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(ref);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_trajectory_make_angle_based(pbio_trajectory_t *ref, int32_t t0, int32_t th0, int32_t th3, int32_t w0, int32_t wt, int32_t wmax, int32_t a) {

    // Return error for zero speed
    if (wt == 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Return empty maneuver for zero angle
    if (th3 == th0) {
        pbio_trajectory_make_stationary(ref, t0, th0, 0);
        return PBIO_SUCCESS;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = th3 < th0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        th3 = 2*th0 - th3;
        w0 *= -1;
    }
    // In a forward maneuver, the target speed is always positive.
    wt = abs(wt);

    // Limit initial speed, but evaluate square root only if necessary (usually not)
    if (w0 > 0 && (w0*w0)/(2*a) > th3 - th0) {
        w0 = pbio_math_sqrt(2*a*(th3 - th0));
    }

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate towards intersection from below,
        // either by reaching constant speed phase or not.
        ref->a0 = a;

        // Fictitious zero speed angle (ahead of us if we have negative initial speed; behind us if we have initial positive speed)
        int32_t thf = th0 - (w0*w0)/(2*a);

        // Test if we can get to ref speed
        if (th3-thf >= (wt*wt)/a) {
            //  If so, find both constant speed intersections
            ref->th1 = thf + (wt*wt)/(2*a);
            ref->th2 = th3 - (wt*wt)/(2*a);
            ref->w1 = wt;
        }
        else {
            // Otherwise, intersect halfway between accelerating and decelerating square root arcs
            ref->th1 = (th3+thf)/2;
            ref->th2 = ref->th1;
            ref->w1 = pbio_math_sqrt(2*a*(ref->th1 - thf));
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate towards intersection from above
        ref->a0 = -a;
        ref->th1 = th0 + (w0*w0-wt*wt)/(2*a);
        ref->th2 = th3 - (wt*wt)/(2*a);
        ref->w1 = wt;
    }
    // Corresponding time intervals
    int32_t t1mt0 = wdiva(ref->w1-w0, ref->a0);
    int32_t t2mt1 = ref->th2 == ref->th1 ? 0 : wdiva(ref->th2-ref->th1, ref->w1);
    int32_t t3mt2 = wdiva(ref->w1, a);

    // Store other results/arguments
    ref->w0 = w0;
    ref->th0 = th0;
    ref->th3 = th3;
    ref->t0 = t0;
    ref->t1 = t0 + t1mt0;
    ref->t2 = ref->t1 + t2mt1;
    ref->t3 = ref->t2 + t3mt2;
    ref->a2 = -a;

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(ref);
    }

    // This is a finite maneuver
    ref->forever = false;

    // Angle based does not use increased resolution yet, but keep scaled values for compatibility
    ref->mth0 = to_mcount(ref->th0);
    ref->mth1 = to_mcount(ref->th1);
    ref->mth2 = to_mcount(ref->th2);
    ref->mth3 = to_mcount(ref->th3);

    return PBIO_SUCCESS;
}

// Evaluate the reference speed and velocity at the (shifted) time
void pbio_trajectory_get_reference(pbio_trajectory_t *traject, int32_t time_ref, int32_t *count_ref, int64_t *mcount_ref, int32_t *rate_ref, int32_t *acceleration_ref) {
    if (time_ref - traject->t1 < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        *rate_ref = traject->w0   + timest(traject->a0, time_ref-traject->t0);
        *mcount_ref = traject->mth0 + x_time(traject->w0, time_ref-traject->t0) + x_time2(traject->a0, time_ref-traject->t0);
        *acceleration_ref = traject->a0;
    }
    else if (traject->forever || time_ref - traject->t2 <= 0) {
        // If we are here, then we are in the constant speed phase
        *rate_ref = traject->w1;
        *mcount_ref = traject->mth1 + x_time(traject->w1, time_ref-traject->t1);
        *acceleration_ref = 0;
    }
    else if (time_ref - traject->t3 <= 0) {
        // If we are here, then we are in the deceleration phase
        *rate_ref = traject->w1 + timest(traject->a2,    time_ref-traject->t2);
        *mcount_ref = traject->mth2  + x_time(traject->w1, time_ref-traject->t2) + x_time2(traject->a2, time_ref-traject->t2);
        *acceleration_ref = traject->a2;
    }
    else {
        // If we are here, we are in the zero speed phase (relevant when holding position)
        *rate_ref = 0;
        *mcount_ref = traject->mth3;
        *acceleration_ref = 0;
    }
    *count_ref = to_count(*mcount_ref);
}
