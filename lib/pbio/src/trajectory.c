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

static void reverse_trajectory(pbio_trajectory_t *trj) {
    // Negate positions, essentially flipping around starting point.
    trj->th1 = -trj->th1;
    trj->th2 = -trj->th2;
    trj->th3 = -trj->th3;
    trj->th1_ext = -trj->th1_ext;
    trj->th2_ext = -trj->th2_ext;
    trj->th3_ext = -trj->th3_ext;

    // Negate speeds and accelerations
    trj->w0 *= -1;
    trj->w1 *= -1;
    trj->w3 *= -1;
    trj->a0 *= -1;
    trj->a2 *= -1;

    // Negate starting point data
    trj->start.rate *= -1;
    trj->start.acceleration *= -1;
}

// Populates the starting point of a trajectory based on user command.
static void pbio_trajectory_set_start(pbio_trajectory_reference_t *start, const pbio_trajectory_command_t *c) {
    start->acceleration = 0;
    start->count = c->angle_start;
    start->count_ext = c->angle_start_ext;
    start->rate = c->speed_start;
    start->time = c->time_start;
}

void pbio_trajectory_make_constant(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Almost everything will be zero, so just zero everything.
    *trj = (pbio_trajectory_t) {0};

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Set speeds.
    trj->w0 = c->speed_target;
    trj->w1 = c->speed_target;
    trj->w3 = c->continue_running ? c->speed_target : 0;
}

static int64_t x_time(int32_t b, int32_t t) {
    return (((int64_t)b) * ((int64_t)t)) / US_PER_MS;
}

static int64_t x_time2(int32_t b, int32_t t) {
    return x_time(x_time(b, t), t) / (2 * US_PER_MS);
}

// Gets the angle traversed while accelerating from w_start to w_end
static int32_t w2_over_a(int32_t w_end, int32_t w_start, int32_t a) {
    return (w_end * w_end - w_start * w_start) / a / 2;
}

// Computes a trajectory for a timed command assuming *positive* speed
static void pbio_trajectory_new_forward_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Save duration.
    trj->t3 = c->duration;

    // Speed continues at target speed or goes to zero.
    trj->w3 = c->continue_running ? c->speed_target : 0;

    // Bind initial speed to make solution feasible.
    trj->w0 = c->speed_start;
    if (trj->w0 * US_PER_SECOND / c->duration < -c->acceleration) {
        trj->w0 = -timest(c->acceleration, c->duration);
    }
    if ((trj->w0 - trj->w3) * US_PER_SECOND / c->duration > max(c->acceleration, c->deceleration)) {
        trj->w0 = trj->w3 + timest(max(c->acceleration, c->deceleration), c->duration);
    }

    // Bind target speed to make solution feasible.
    int32_t wt = c->speed_target;
    if ((wt - trj->w3) * US_PER_SECOND / c->duration > c->deceleration) {
        wt = trj->w3 + timest(c->deceleration, c->duration);
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? c->acceleration : -c->acceleration;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -c->deceleration;

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt2;
    int32_t t2mt1;

    // The in-phase completes at the point where it reaches the target speed.
    trj->t1 = wdiva(wt - trj->w0, trj->a0);

    // The out-phase rotation is the time passed while slowing down.
    t3mt2 = wdiva(trj->w3 - wt, trj->a2);

    // Get the time and speed for the constant speed phase.
    t2mt1 = trj->t3 - trj->t1 - t3mt2;
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
            trj->t1 = trj->t3;
            t2mt1 = 0;
            t3mt2 = 0;
            trj->w1 = trj->w0 + timest(trj->a0, trj->t3);
            trj->w3 = trj->w1;
        } else {
            // Otherwise, we can just take the intersection of the accelerating
            // and decelerating ramps to find the speed at t1 = t2.
            assert(trj->a0 != trj->a2);
            trj->t1 = wdiva(trj->w3 - trj->w0 - timest(trj->a2, trj->t3), trj->a0 - trj->a2);

            // There is no constant speed phase in this case.
            t2mt1 = 0;
            t3mt2 = trj->t3 - trj->t1;
            trj->w1 = trj->w0 + timest(trj->a0, trj->t1);
        }
    }

    // With the difference between t1 and t2 known, we know t2.
    trj->t2 = trj->t1 + t2mt1;

    // Corresponding angle values
    trj->th1 = w2_over_a(trj->w1, trj->w0, trj->a0);
    trj->th2 = trj->th1 + (trj->w1 * (t2mt1 / US_PER_MS)) / MS_PER_SECOND;
    trj->th3 = trj->th2 + w2_over_a(trj->w3, trj->w1, trj->a2);
}

// Computes a trajectory for an angle command assuming *positive* speed
static void pbio_trajectory_new_forward_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // The given angle constraints won't change, so store them right away.
    trj->th3 = c->angle_end - c->angle_start;

    // Speed continues at target speed or goes to zero.
    trj->w3 = c->continue_running ? c->speed_target : 0;

    // Revisit: Drop ext and switch to increased angle resolution everywhere.
    trj->th1_ext = 0;
    trj->th2_ext = 0;
    trj->th3_ext = 0;

    // Bind initial speed to make solution feasible. Do the larger-than check
    // using quadratic terms to avoid square root evaluations in most cases.
    // This is only needed for positive initial speed, because negative initial
    // speeds are always feasible in angle-based maneuvers.
    int32_t a_max = max(c->acceleration, c->deceleration);
    trj->w0 = c->speed_start;
    if (trj->w0 > 0 && trj->w0 * trj->w0 - trj->w3 * trj->w3 > 2 * a_max * trj->th3) {
        trj->w0 = pbio_math_sqrt(trj->w3 * trj->w3 + 2 * a_max * trj->th3);
    }

    // Do the same check for the target speed, but now use the deceleration
    // magnitude as the only constraint. The target speed is always positive,
    // so we can omit that additional check here.
    int32_t wt = c->speed_target;
    if (wt * wt - trj->w3 * trj->w3 > 2 * c->deceleration * trj->th3) {
        wt = pbio_math_sqrt(trj->w3 * trj->w3 + 2 * c->deceleration * trj->th3);
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? c->acceleration : -c->acceleration;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -c->deceleration;

    // Now we can evaluate the nominal cases and check if they hold. First get
    // a fictitious zero-speed angle for computational convenience. This is the
    // angle on the speed-angle phase plot where the in-phase square root
    // function intersects the zero speed axis if we kept (de)accelerating.
    int32_t thf = -trj->w0 * trj->w0 / (2 * trj->a0);

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
    trj->t1 = wdiva(trj->w1 - trj->w0, trj->a0);
    trj->t2 = trj->t1 + wdiva(trj->th2 - trj->th1, trj->w1);
    trj->t3 = trj->t2 + wdiva(trj->w3 - trj->w1, trj->a2);
}

void pbio_trajectory_stretch(pbio_trajectory_t *trj, pbio_trajectory_t *leader) {

    // Synchronize timestamps with leading trajectory.
    trj->t1 = leader->t1;
    trj->t2 = leader->t2;
    trj->t3 = leader->t3;

    if (trj->t3 == 0) {
        // This is a stationary maneuver, so there's nothing to recompute.
        return;
    }

    // This recomputes several components of a trajectory such that it travels
    // the same distance as before, but with new time stamps t1, t2, and t3.
    // Setting the speed integral equal to (th3 - th0) gives three constraint
    // equations with three unknowns (a0, a2, wt), for which we can solve.

    // Solve constraint to find peak velocity
    trj->w1 = (2 * trj->th3 - timest(trj->w0, trj->t1) - timest(trj->w3, trj->t3 - trj->t2)) * US_PER_MS /
        ((trj->t3 + trj->t2 - trj->t1) / US_PER_MS);

    // Get corresponding accelerations
    trj->a0 = trj->t1 == 0 ? 0 : (trj->w1 - trj->w0) * US_PER_MS / trj->t1 * MS_PER_SECOND;
    trj->a2 = (trj->t3 - trj->t2) == 0 ? 0 : (trj->w3 - trj->w1) * US_PER_MS / (trj->t3 - trj->t2) * MS_PER_SECOND;

    // Since the target speed may have been lowered, we need to adjust w3 too.
    trj->w3 = (trj->t3 - trj->t2) == 0 ? trj->w1 : 0;

    // With all constraints already satisfied, we can just compute the
    // intermediate positions relative to the endpoints, given the now-known
    // accelerations and speeds.
    trj->th1 = timest(trj->w0, trj->t1) + timest2(trj->a0, trj->t1);
    trj->th2 = trj->th1 + timest(trj->w1, trj->t2 - trj->t1);
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
        c.speed_target = 0;
        c.continue_running = false;
        pbio_trajectory_make_constant(trj, &c);
        return PBIO_SUCCESS;
    }

    // Remember if the original user-specified maneuver was backward.
    bool backward = c.speed_target < 0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c.speed_target *= -1;
        c.speed_start *= -1;
    }

    // Bind target speed by maximum speed.
    c.speed_target = min(c.speed_target, c.speed_max);

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_time_command(trj, &c);

    // Assert that all resulting time intervals are positive.
    if (trj->t1 < 0 || trj->t2 - trj->t1 < 0 || trj->t3 - trj->t2 < 0) {
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
    if (abs((c.angle_end - c.angle_start) / c.speed_target) + 1 > DURATION_MAX_MS / 1000) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Return empty maneuver for zero angle or zero speed
    if (c.angle_end == c.angle_start || c.speed_target == 0) {
        c.speed_target = 0;
        c.continue_running = false;
        pbio_trajectory_make_constant(trj, &c);
        return PBIO_SUCCESS;
    }

    // Direction is solely defined in terms of th3 position relative to th0.
    // For speed, only the *magnitude* is relevant. Certain end-user APIs
    // allow specifying phyically impossible scenarios like negative speed
    // with a positive relative position. Those cases are not handled here and
    // should be appropriately handled at higher levels.
    c.speed_target = abs(c.speed_target);

    // Bind target speed by maximum speed.
    c.speed_target = min(c.speed_target, c.speed_max);

    // Check if the original user-specified maneuver is backward.
    bool backward = c.angle_end < c.angle_start;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c.angle_end = 2 * c.angle_start - c.angle_end;
        c.speed_start *= -1;
    }

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_angle_command(trj, &c);

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }

    return PBIO_SUCCESS;
}

// Adds starting point to newly computed relative reference, so we have an
// absolute position again for use outside this code module.
static void pbio_trajectory_offset_start(pbio_trajectory_reference_t *ref, pbio_trajectory_reference_t *start) {
    ref->count += start->count;

    // REVISIT: We will drop this on converting the units, so we don't
    // introduce a dedicated add function for this 64-bit-like type.
    ref->count_ext += start->count_ext;
    if (ref->count_ext > 1000) {
        ref->count_ext -= 1000;
        ref->count += 1;
    } else if (ref->count_ext < -1000) {
        ref->count_ext += 1000;
        ref->count -= 1;
    }
    ref->time += start->time;
    // Acceleration and speed are not offset, so we don't need to change those.
}

void pbio_trajectory_get_last_vertex(pbio_trajectory_t *trj, int32_t time_ref, pbio_trajectory_reference_t *vertex) {

    // Relative time within ongoing manauver.
    int32_t time = time_ref - trj->start.time;

    // Find which section of the ongoing maneuver we were in, and take
    // corresponding segment starting point.
    if (time - trj->t1 < 0) {
        // Acceleration segment.
        vertex->time = 0;
        vertex->rate = trj->w0;
        vertex->count = 0;
        vertex->count_ext = 0;
    } else if (time - trj->t2 < 0) {
        // Constant speed segment.
        vertex->time = trj->t1;
        vertex->rate = trj->w1;
        vertex->count = trj->th1;
        vertex->count_ext = trj->th1_ext;
    } else if (time - trj->t3 < 0) {
        // Deceleration segment.
        vertex->time = trj->t2;
        vertex->rate = trj->w1;
        vertex->count = trj->th2;
        vertex->count_ext = trj->th2_ext;
    } else {
        // Final speed segment.
        vertex->time = trj->t3;
        vertex->rate = trj->w3;
        vertex->count = trj->th3;
        vertex->count_ext = trj->th3_ext;
    }

    // Convert back to absolute points by adding starting point.
    pbio_trajectory_offset_start(vertex, &trj->start);
}

// Get trajectory endpoint.
void pbio_trajectory_get_endpoint(pbio_trajectory_t *trj, pbio_trajectory_reference_t *end) {
    // Get endpoint with respect to stary.
    end->time = trj->t3;
    end->count = trj->th3;
    end->count_ext = 0;
    end->rate = trj->w3;
    end->acceleration = 0;

    // Convert back to absolute points by adding starting point.
    pbio_trajectory_offset_start(end, &trj->start);
}

// Get trajectory endpoint.
int32_t pbio_trajectory_get_duration(pbio_trajectory_t *trj) {
    return trj->t3;
}

// Evaluate the reference speed and velocity at the (shifted) time
void pbio_trajectory_get_reference(pbio_trajectory_t *trj, int32_t time_ref, pbio_trajectory_reference_t *ref) {

    // Wall time of reference evaluation.
    ref->time = time_ref;

    // Time within maneuver, so time since start.
    int32_t time = time_ref - trj->start.time;

    int64_t mcount_ref;

    if (time - trj->t1 < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        ref->rate = trj->w0 + timest(trj->a0, time);
        mcount_ref = x_time(trj->w0, time) + x_time2(trj->a0, time);
        ref->acceleration = trj->a0;
    } else if (time - trj->t2 <= 0) {
        // If we are here, then we are in the constant speed phase
        ref->rate = trj->w1;
        mcount_ref = as_mcount(trj->th1, trj->th1_ext) + x_time(trj->w1, time - trj->t1);
        ref->acceleration = 0;
    } else if (time - trj->t3 <= 0) {
        // If we are here, then we are in the deceleration phase
        ref->rate = trj->w1 + timest(trj->a2, time - trj->t2);
        mcount_ref = as_mcount(trj->th2, trj->th2_ext) + x_time(trj->w1, time - trj->t2) + x_time2(trj->a2, time - trj->t2);
        ref->acceleration = trj->a2;
    } else {
        // If we are here, we are in the constant speed phase after the maneuver completes
        ref->rate = trj->w3;
        mcount_ref = as_mcount(trj->th3, trj->th3_ext) + x_time(trj->w3, time - trj->t3);
        ref->acceleration = 0;

        // To avoid any overflows of the aforementioned time comparisons,
        // rebase the trajectory if it has been running a long time.
        if (time > DURATION_FOREVER_MS * US_PER_MS) {
            pbio_trajectory_command_t command = {
                .time_start = time,
                .speed_target = trj->w3,
                .continue_running = true,
            };
            as_count(mcount_ref, &command.angle_start, &command.angle_start_ext);
            pbio_trajectory_make_constant(trj, &command);
        }
    }

    // Split high res angle into counts and millicounts
    as_count(mcount_ref, &ref->count, &ref->count_ext);

    // Convert back to absolute points by adding starting point.
    pbio_trajectory_offset_start(ref, &trj->start);
}
