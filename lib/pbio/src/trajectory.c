// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/angle.h>
#include <pbio/math.h>
#include <pbio/trajectory.h>

/**
 * Position (mdeg) and time (1e-4 s) are the same as in control module.
 * But speed is in millidegrees/second in control units, but this module uses
 * decidegrees per second to keep the math numerically bounded.
 */
#define TO_TRAJECTORY_SPEED(speed) ((speed) / 100)
#define TO_CONTROL_SPEED(speed) ((speed) * 100)

/**
 * Acceleration is in millidegrees/second^2 in control units, but this module
 * uses deg/s^2 per second to keep the math numerically bounded.
 */
#define TO_TRAJECTORY_ACCEL(acc) ((acc) / 1000)
#define TO_CONTROL_ACCEL(acc) ((acc) * 1000)

/**
 * Time is unsigned everywhere except in the trajectory module.
 */
#define TO_TRAJECTORY_TIME(time) ((int32_t)(time))
#define TO_CONTROL_TIME(time) ((uint32_t)(time))

static void reverse_trajectory(pbio_trajectory_t *trj) {
    // Negate positions, essentially flipping around starting point.
    trj->th1 = -trj->th1;
    trj->th2 = -trj->th2;
    trj->th3 = -trj->th3;

    // Negate speeds and accelerations
    trj->w0 *= -1;
    trj->w1 *= -1;
    trj->w3 *= -1;
    trj->a0 *= -1;
    trj->a2 *= -1;

    // Negate starting point data
    trj->start.speed *= -1;
    trj->start.acceleration *= -1;
}

// Populates the starting point of a trajectory based on user command.
static void pbio_trajectory_set_start(pbio_trajectory_reference_t *start, const pbio_trajectory_command_t *c) {
    start->acceleration = 0;
    start->position = c->position_start;
    start->speed = c->speed_start;
    start->time = c->time_start;
}

void pbio_trajectory_make_constant(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Almost everything will be zero, so just zero everything.
    *trj = (pbio_trajectory_t) {0};

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Set speeds, scaled to ddeg/s.
    trj->w0 = TO_TRAJECTORY_SPEED(c->speed_target);
    trj->w1 = TO_TRAJECTORY_SPEED(c->speed_target);
    trj->w3 = c->continue_running ? TO_TRAJECTORY_SPEED(c->speed_target): 0;
}

// Divides speed^2 (ddeg/s)^2 by acceleration (deg/s^2)*2, giving angle (mdeg).
static int32_t div_w2_by_a(int32_t w_end, int32_t w_start, int32_t a) {
    int32_t product = w_end * w_end - w_start * w_start;
    if (pbio_math_abs(product) > INT32_MAX / (10 / 2)) {
        // Divide first if we have to.
        return product / a * (10 / 2);
    }
    // Higher resolution result if possible.
    return product * (10 / 2) / a;
}

// Divides speed (ddeg/s) by acceleration (deg/s^2), giving time (s e-4).
static int32_t div_w_by_a(int32_t w, int32_t a) {
    return w * 1000 / a;
}

// Divides angle (mdeg) by time (s e-4), giving speed (ddeg/s).
static int32_t div_th_by_t(int32_t th, int32_t t) {
    if (pbio_math_abs(th) < INT32_MAX / 100) {
        return th * 100 / t;
    }
    return th / t * 100;
}

// Divides speed (ddeg/s) by time (s e-4), giving acceleration (deg/s^2).
static int32_t div_w_by_t(int32_t w, int32_t t) {
    return w * 1000 / t;
}

// Divides angle (mdeg) by speed (deg/s), giving time (s e-4).
static int32_t div_th_by_w(int32_t th, int32_t w) {
    if (pbio_math_abs(th) < INT32_MAX / 100) {
        return th * 100 / w;
    }
    return th / w * 100;
}

// Multiplies speed (ddeg/s) by time (s e-4), giving angle (mdeg).
static int32_t mul_w_by_t(int32_t w, int32_t t) {
    // Get safe result first in case a long time is used.
    int32_t result = w * (t / 100);
    if (pbio_math_abs(result) > (INT32_MAX / 100)) {
        return result;
    }
    // Get more accurate result if we know the product does not overflow.
    return w * t / 100;
}

// Multiplies acceleration (deg/s^2) by time (s e-4), giving speed (ddeg/s).
static int32_t mul_a_by_t(int32_t a, int32_t t) {
    // Get safe result first in case a long time is used.
    int32_t result = a * (t / 1000);
    if (pbio_math_abs(result) > INT32_MAX / 1000) {
        return result;
    }
    // Get more accurate result if we know the product does not overflow.
    return a * t / 1000;
}

// Multiplies acceleration (deg/s^2) by time (s e-4)^2/2, giving angle (mdeg).
static int32_t mul_a_by_t2(int32_t a, int32_t t) {
    return mul_w_by_t(mul_a_by_t(a, t), t) / 2;
}

// Gets starting speed (ddeg/s) to reach end speed (ddeg/s) within given
// angle (mdeg) and acceleration (deg/s^2). Inverse of div_w2_by_a.
static int32_t bind_w0(int32_t w_end, int32_t a, int32_t th) {
    // Evaluates sqrt(w_end^2 + 2 * a * th), with scaling for units.
    return pbio_math_sqrt(w_end * w_end + a * th / (10 / 2));
}

// Given a stationary starting and final angle (mdeg), computes the
// intersection of the speed curves if we accelerate (deg/s^2) up and down
// without a stationary speed phase.
static int32_t intersect_ramp(int32_t th3, int32_t th0, int32_t a0, int32_t a2) {
    if (pbio_math_abs(th3 - th0) > INT32_MAX / pbio_math_abs(a2)) {
        // Divide first if numerator too large.
        return th0 + (th3 - th0) / (a2 - a0) * a2;
    }
    // Higher resolution result for small angles.
    return th0 + (th3 - th0) * a2 / (a2 - a0);
}

// Computes a trajectory for a timed command assuming *positive* speed
static void pbio_trajectory_new_forward_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Save duration.
    trj->t3 = TO_TRAJECTORY_TIME(c->duration);

    // Speed continues at target speed or goes to zero. And scale to ddeg/s.
    trj->w3 = c->continue_running ? TO_TRAJECTORY_SPEED(c->speed_target) : 0;
    trj->w0 = TO_TRAJECTORY_SPEED(c->speed_start);
    int32_t wt = TO_TRAJECTORY_SPEED(c->speed_target);
    int32_t accel = TO_TRAJECTORY_ACCEL(c->acceleration);
    int32_t decel = TO_TRAJECTORY_ACCEL(c->deceleration);

    // Bind initial speed to make solution feasible.
    if (div_w_by_a(trj->w0, accel) < -trj->t3) {
        trj->w0 = -mul_a_by_t(accel, trj->t3);
    }
    if (div_w_by_a(trj->w0 - trj->w3, pbio_math_max(accel, decel)) > trj->t3) {
        trj->w0 = trj->w3 + mul_a_by_t(pbio_math_max(accel, decel), trj->t3);
    }

    // Bind target speed to make solution feasible.
    if (div_w_by_a(wt - trj->w3, decel) > trj->t3) {
        wt = trj->w3 + mul_a_by_t(decel, trj->t3);
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? accel : -accel;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -decel;

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    int32_t t3mt2;
    int32_t t2mt1;

    // The in-phase completes at the point where it reaches the target speed.
    trj->t1 = div_w_by_a(wt - trj->w0, trj->a0);

    // The out-phase rotation is the time passed while slowing down.
    t3mt2 = div_w_by_a(trj->w3 - wt, trj->a2);

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
            trj->w1 = trj->w0 + mul_a_by_t(trj->a0, trj->t3);
            trj->w3 = trj->w1;
        } else {
            // Otherwise, we can just take the intersection of the accelerating
            // and decelerating ramps to find the angle at t1 = t2.
            assert(trj->a0 != trj->a2);
            trj->t1 = div_w_by_a(trj->w3 - trj->w0 - mul_a_by_t(trj->a2, trj->t3), trj->a0 - trj->a2);

            // There is no constant speed phase in this case.
            t2mt1 = 0;
            t3mt2 = trj->t3 - trj->t1;
            trj->w1 = trj->w0 + mul_a_by_t(trj->a0, trj->t1);
        }
    }

    // With the difference between t1 and t2 known, we know t2.
    trj->t2 = trj->t1 + t2mt1;

    // Corresponding angle values
    trj->th1 = div_w2_by_a(trj->w1, trj->w0, trj->a0);
    trj->th2 = trj->th1 + mul_w_by_t(trj->w1, t2mt1);
    trj->th3 = trj->th2 + div_w2_by_a(trj->w3, trj->w1, trj->a2);
}

// Computes a trajectory for an angle command assuming *positive* speed
static void pbio_trajectory_new_forward_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Get angle to travel.
    trj->th3 = pbio_angle_diff_mdeg((pbio_angle_t *)&c->position_end, (pbio_angle_t *)&c->position_start);

    // Speed continues at target speed or goes to zero. And scale to ddeg/s.
    trj->w3 = c->continue_running ? TO_TRAJECTORY_SPEED(c->speed_target) : 0;
    trj->w0 = TO_TRAJECTORY_SPEED(c->speed_start);
    int32_t wt = TO_TRAJECTORY_SPEED(c->speed_target);
    int32_t accel = TO_TRAJECTORY_ACCEL(c->acceleration);
    int32_t decel = TO_TRAJECTORY_ACCEL(c->deceleration);

    // Bind initial speed to make solution feasible. Do the larger-than check
    // using quadratic terms to avoid square root evaluations in most cases.
    // This is only needed for positive initial speed, because negative initial
    // speeds are always feasible in angle-based maneuvers.
    int32_t a_max = pbio_math_max(accel, decel);
    if (trj->w0 > 0 && div_w2_by_a(trj->w0, trj->w3, a_max) > trj->th3) {
        trj->w0 = bind_w0(trj->w3, a_max, trj->th3);
    }

    // Do the same check for the target speed, but now use the deceleration
    // magnitude as the only constraint. The target speed is always positive,
    // so we can omit that additional check here.
    if (div_w2_by_a(wt, trj->w3, decel) > trj->th3) {
        wt = bind_w0(trj->w3, decel, trj->th3);
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsicaly dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? accel : -accel;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -decel;

    // Now we can evaluate the nominal cases and check if they hold. First get
    // a fictitious zero-speed angle for computational convenience. This is the
    // angle on the speed-angle phase plot where the in-phase square root
    // function intersects the zero speed axis if we kept (de)accelerating.
    int32_t thf = div_w2_by_a(0, -trj->w0, trj->a0);

    // The in-phase completes at the point where it reaches the target speed.
    trj->th1 = thf + div_w2_by_a(wt, 0, trj->a0);

    // The out-phase rotation is the angle traveled while slowing down. But if
    // the end speed equals the target speed, there is no out-phase.
    trj->th2 = trj->th3 + div_w2_by_a(wt, trj->w3, trj->a2);

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
            trj->w1 = bind_w0(0, trj->a0, trj->th3 - thf);
            trj->th1 = trj->th3;
            trj->th2 = trj->th1;

            // The final speed is not reached either, so reaching
            // w1 is the best we can do in the given time.
            trj->w3 = trj->w1;
        } else {
            // Otherwise, we can just take the intersection of the accelerating
            // and decelerating ramps to find the speed at t1 = t2.
            trj->th1 = intersect_ramp(trj->th3, thf, trj->a0, trj->a2);
            trj->th2 = trj->th1;
            trj->w1 = bind_w0(0, trj->a0, trj->th1 - thf);
        }
    }

    // With the intermediate angles and speeds now known, we can calculate the
    // corresponding durations to match.
    trj->t1 = div_w_by_a(trj->w1 - trj->w0, trj->a0);
    trj->t2 = trj->t1 + div_th_by_w(trj->th2 - trj->th1, trj->w1);
    trj->t3 = trj->t2 + div_w_by_a(trj->w3 - trj->w1, trj->a2);
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
    trj->w1 = div_th_by_t(2 * trj->th3 - mul_w_by_t(trj->w0, trj->t1) - mul_w_by_t(trj->w3, trj->t3 - trj->t2),
        trj->t3 + trj->t2 - trj->t1);

    // Get corresponding accelerations
    trj->a0 = trj->t1 == 0 ? 0 : div_w_by_t(trj->w1 - trj->w0, trj->t1);
    trj->a2 = (trj->t3 - trj->t2) == 0 ? 0 : div_w_by_t(trj->w3 - trj->w1, trj->t3 - trj->t2);

    // Since the target speed may have been lowered, we need to adjust w3 too.
    trj->w3 = (trj->t3 - trj->t2) == 0 ? trj->w1 : 0;

    // With all constraints already satisfied, we can just compute the
    // intermediate positions relative to the endpoints, given the now-known
    // accelerations and speeds.
    trj->th1 = mul_w_by_t(trj->w0, trj->t1) + mul_a_by_t2(trj->a0, trj->t1);
    trj->th2 = trj->th1 + mul_w_by_t(trj->w1, trj->t2 - trj->t1);
}

pbio_error_t pbio_trajectory_new_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command) {

    // Copy the command so we can modify it.
    pbio_trajectory_command_t c = *command;

    // Return error for a long user-specified duration. Too long is defined as
    // taking as long as the maximum allowed millidegrees distance.
    int32_t approx_deg = (c.duration / 10000) * (pbio_math_abs(c.speed_target) / 1000);
    if (approx_deg > INT32_MAX / 1000) {
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
    c.speed_target = pbio_math_min(c.speed_target, c.speed_max);

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
    if (!pbio_angle_diff_is_small(&c.position_end, &c.position_start)) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Travel distance.
    int32_t distance = pbio_angle_diff_mdeg(&c.position_end, &c.position_start);

    // Return empty maneuver for zero angle or zero speed
    if (c.speed_target == 0 || distance == 0) {
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
    c.speed_target = pbio_math_abs(c.speed_target);

    // Bind target speed by maximum speed.
    c.speed_target = pbio_math_min(c.speed_target, c.speed_max);

    // Check if the original user-specified maneuver is backward.
    bool backward = distance < 0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c.position_end = c.position_start;
        pbio_angle_add_mdeg(&c.position_end, -distance);
    }

    // Calculate the trajectory, assumed to be forward.
    pbio_trajectory_new_forward_angle_command(trj, &c);

    // Ensure computed motion points are ordered.
    if (trj->th1 < 0 || trj->th2 < trj->th1 || trj->th3 < trj->th2) {
        return PBIO_ERROR_FAILED;
    }

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }

    return PBIO_SUCCESS;
}

// Pupulates reference point with the right units and offset
static void pbio_trajectory_offset_start(pbio_trajectory_reference_t *ref, pbio_trajectory_reference_t *start, int32_t t, int32_t th, int32_t w, int32_t a) {

    // Convert local trajectory units to global pbio units.
    ref->time = TO_CONTROL_TIME(t) + start->time;
    ref->speed = TO_CONTROL_SPEED(w);
    ref->acceleration = TO_CONTROL_ACCEL(a);

    // Reference position is starting point plus progress in this maneuver.
    ref->position = start->position;
    pbio_angle_add_mdeg(&ref->position, th);
}

void pbio_trajectory_get_last_vertex(pbio_trajectory_t *trj, uint32_t time_ref, pbio_trajectory_reference_t *vertex) {

    // Relative time within ongoing manauver.
    int32_t time = TO_TRAJECTORY_TIME(time_ref - trj->start.time);

    // Find which section of the ongoing maneuver we were in, and take
    // corresponding segment starting point. Acceleration is undefined but not
    // used when synchronizing trajectories, so set to zero.
    if (time - trj->t1 < 0) {
        // Acceleration segment.
        pbio_trajectory_offset_start(vertex, &trj->start, 0, 0, trj->w0, 0);
    } else if (time - trj->t2 < 0) {
        // Constant speed segment.
        pbio_trajectory_offset_start(vertex, &trj->start, trj->t1, trj->th1, trj->w1, 0);
    } else if (time - trj->t3 < 0) {
        // Deceleration segment.
        pbio_trajectory_offset_start(vertex, &trj->start, trj->t2, trj->th2, trj->w1, 0);
    } else {
        // Final speed segment.
        pbio_trajectory_offset_start(vertex, &trj->start, trj->t3, trj->th3, trj->w3, 0);
    }
}

// Get trajectory endpoint.
void pbio_trajectory_get_endpoint(pbio_trajectory_t *trj, pbio_trajectory_reference_t *end) {
    pbio_trajectory_offset_start(end, &trj->start, trj->t3, trj->th3, trj->w3, 0);
}

// Get trajectory endpoint.
uint32_t pbio_trajectory_get_duration(pbio_trajectory_t *trj) {
    return TO_CONTROL_TIME(trj->t3);
}

// Evaluate the reference speed and velocity at the (shifted) time
void pbio_trajectory_get_reference(pbio_trajectory_t *trj, uint32_t time_ref, pbio_trajectory_reference_t *ref) {

    // Time within maneuver since start.
    int32_t time = TO_TRAJECTORY_TIME(time_ref - trj->start.time);

    // Get angle, speed, and acceleration along reference
    int32_t th, w, a;

    if (time - trj->t1 < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        w = trj->w0 + mul_a_by_t(trj->a0, time);
        th = mul_w_by_t(trj->w0, time) + mul_a_by_t2(trj->a0, time);
        a = trj->a0;
    } else if (time - trj->t2 <= 0) {
        // If we are here, then we are in the constant speed phase
        w = trj->w1;
        th = trj->th1 + mul_w_by_t(trj->w1, time - trj->t1);
        a = 0;
    } else if (time - trj->t3 <= 0) {
        // If we are here, then we are in the deceleration phase
        w = trj->w1 + mul_a_by_t(trj->a2, time - trj->t2);
        th = trj->th2 + mul_w_by_t(trj->w1, time - trj->t2) + mul_a_by_t2(trj->a2, time - trj->t2);
        a = trj->a2;
    } else {
        // If we are here, we are in the constant speed phase after the maneuver completes
        w = trj->w3;
        th = trj->th3 + mul_w_by_t(trj->w3, time - trj->t3);
        a = 0;

        // To avoid any overflows of the aforementioned time comparisons,
        // rebase the trajectory if it has been running a long time.
        if (time > DURATION_FOREVER_MS * 10) {
            pbio_trajectory_command_t command = {
                .time_start = time_ref,
                .speed_target = TO_CONTROL_SPEED(trj->w3),
                .continue_running = true,
            };
            pbio_trajectory_make_constant(trj, &command);
        }
    }

    // Convert back to absolute points by adding starting point.
    pbio_trajectory_offset_start(ref, &trj->start, time, th, w, a);
}
