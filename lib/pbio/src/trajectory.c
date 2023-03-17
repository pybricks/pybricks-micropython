// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/angle.h>
#include <pbio/int_math.h>
#include <pbio/trajectory.h>

/**
 * For a single maneuver, the relative angle in millidegrees is capped at
 * half the numerical limit.
 */
#define ANGLE_MAX (INT32_MAX / 2)
#define assert_angle(th) (assert(pbio_int_math_abs((th)) < ANGLE_MAX))

/**
 * Speed is capped at 2000 deg/s (20000 ddeg/s), which is about twice the
 * rated speed for a typical motor. Some commands deal with speeds relative
 * to another, so allow up to double that amount.
 */
#define SPEED_MAX (20000)
#define assert_speed(w) (assert(pbio_int_math_abs((w)) <= SPEED_MAX + 1))
#define assert_speed_rel(w) (assert_speed(w / 2))

/**
 * Acceleration is capped at 20000 deg/s^2, which for a typical motor
 * means reaching the maximum speed in just one control sample. The
 * minimum sets an upper bound on the acceleration time.
 */
#define ACCELERATION_MAX (20000)
#define ACCELERATION_MIN (50)
#define ANGLE_ACCEL_MAX (SPEED_MAX * SPEED_MAX / ACCELERATION_MIN * (10 / 2))
#define assert_accel_small(a) (assert(pbio_int_math_abs((a)) <= ACCELERATION_MAX))
#define assert_accel_numerator(a) (assert(pbio_int_math_abs((a)) <= ACCELERATION_MAX && pbio_int_math_abs((a)) >= ACCELERATION_MIN))
#define assert_accel_angle(th) (assert(pbio_int_math_abs((th)) <= ANGLE_ACCEL_MAX))

/*
 * Time segment length is capped at the maximum angle divided by maximum speed,
 * and scaled to time ticks. This is about 536 seconds.
 * Acceleration time segments take at most as long as reaching the maximum
 * speed with the lowest possible acceleration
 */

#define TIME_MAX (ANGLE_MAX / (SPEED_MAX * 100) * 10000)
#define TIME_ACCEL_MAX (SPEED_MAX * 2 / ACCELERATION_MIN * 1000)
#define assert_time(t) (assert((t) >= 0 && (t) < TIME_MAX))
#define assert_accel_time(t) (assert((t) >= 0 && (t) < TIME_ACCEL_MAX))

/*
 * Position (mdeg) and time (1e-4 s) are the same as in control module.
 * But speed is in millidegrees/second in control units, but this module uses
 * decidegrees per second to keep the math numerically bounded.
 */

/**
 * Converts a speed from ddeg/s to mdeg/s.
 *
 * @param [in]  trajectory_speed    The speed in ddeg/s.
 * @returns                         The speed in mdeg/s.
 */
static int32_t to_control_speed(int32_t trajectory_speed) {
    return trajectory_speed * 100;
}

/**
 * Converts a speed from mdeg/s to ddeg/s and limits it to +/- ::SPEED_MAX.
 *
 * @param [in]  control_speed       The speed in mdeg/s.
 * @returns                         The speed in ddeg/s.
 */
static int32_t to_trajectory_speed(int32_t control_speed) {
    return pbio_int_math_clamp(control_speed / 100, SPEED_MAX);
}

/*
 * Acceleration is in millidegrees/second^2 in control units, but this module
 * uses deg/s^2 per second to keep the math numerically bounded.
 */

/**
 * Converts an acceleration from deg/s^2 to mdeg/s^2.
 *
 * @param [in]  trajectory_accel    The acceleration in deg/s^2.
 * @returns                         The acceleration in mdeg/s^2.
 */
static int32_t to_control_accel(int32_t trajectory_accel) {
    return trajectory_accel * 1000;
}

/**
 * Converts an acceleration from mdeg/s^2 to deg/s^2 and limits it to the range
 * ::ACCELERATION_MIN to ::ACCELERATION_MAX.
 *
 * @param [in]  control_accel       The acceleration in mdeg/s^2.
 * @returns                         The acceleration in deg/s^2.
 */
static int32_t to_trajectory_accel(int32_t control_accel) {
    return pbio_int_math_bind(control_accel / 1000, ACCELERATION_MIN, ACCELERATION_MAX);
}

/**
 * Converts time from unsigned (for use outside this file) to signed (used here).
 * @param [in]  time    Unsigned time value.
 * @returns             Signed time value.
 */
#define TO_TRAJECTORY_TIME(time) ((int32_t)(time))

/**
 * Converts time from signed (used here) to unsigned(for use outside this file).
 * @param [in]  time    Signed time value.
 * @returns             Unsigned time value.
 */
#define TO_CONTROL_TIME(time) ((uint32_t)(time))

/**
 * Validates that the given speed limit is within the numerically allowed range.
 *
 * @param [in] ctl_steps_per_app_step   Control units per application unit.
 * @param [in] speed                    Speed limit (application units, positive).
 * @return                              ::PBIO_SUCCESS on valid values.
 *                                      ::PBIO_ERROR_INVALID_ARG if any argument is outside the allowed range.
 */
pbio_error_t pbio_trajectory_validate_speed_limit(int32_t ctl_steps_per_app_step, int32_t speed) {

    // Speeds can be negative but speed *limit* must be a strictly positive value.
    const int32_t speed_min = to_control_speed(100) / ctl_steps_per_app_step;
    const int32_t speed_max = to_control_speed(SPEED_MAX) / ctl_steps_per_app_step;

    if (speed < speed_min || speed > speed_max) {
        return PBIO_ERROR_INVALID_ARG;
    }
    return PBIO_SUCCESS;
}

/**
 * Validates that the given acceleration is within the numerically allowed range.
 *
 * @param [in] ctl_steps_per_app_step   Control units per application unit.
 * @param [in] acceleration             Acceleration (application units, positive).
 * @return                              ::PBIO_SUCCESS on valid values.
 *                                      ::PBIO_ERROR_INVALID_ARG if any argument is outside the allowed range.
 */
pbio_error_t pbio_trajectory_validate_acceleration_limit(int32_t ctl_steps_per_app_step, int32_t acceleration) {
    const int32_t accel_min = to_control_accel(ACCELERATION_MIN) / ctl_steps_per_app_step;
    const int32_t accel_max = to_control_accel(ACCELERATION_MAX) / ctl_steps_per_app_step;
    if (acceleration < accel_min || acceleration > accel_max) {
        return PBIO_ERROR_INVALID_ARG;
    }
    return PBIO_SUCCESS;
}

/**
 * Reverses a trajectory.
 *
 * On return, @p trj has been modified to contain the reverse trajectory.
 *
 * @param trj [in]      An initalized trajectory to be reversed.
 */
static void reverse_trajectory(pbio_trajectory_t *trj) {
    // Negate positions, essentially flipping around starting point.
    trj->th1 = -trj->th1;
    trj->th2 = -trj->th2;
    trj->th3 = -trj->th3;

    // Negate speeds and accelerations
    trj->wu *= -1;
    trj->w0 *= -1;
    trj->w1 *= -1;
    trj->w3 *= -1;
    trj->a0 *= -1;
    trj->a2 *= -1;

    // Negate starting point data
    trj->start.speed *= -1;
    trj->start.acceleration *= -1;
}

/**
 * Populates the starting point of a trajectory based on user command.
 *
 * @param [out] start   An uninitialized trajectory reference point to hold the result.
 * @param [in]  c       The command to use.
 */
static void pbio_trajectory_set_start(pbio_trajectory_reference_t *start, const pbio_trajectory_command_t *c) {
    start->acceleration = 0;
    start->position = c->position_start;
    start->speed = c->speed_start;
    start->time = c->time_start;
}

/**
 * Initializes a trajectory struct based on a user command.
 *
 * @param [out] trj     An uninitialized trajectory to hold the result.
 * @param [in]  c       The command to use.
 */
void pbio_trajectory_make_constant(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Almost everything will be zero, so just zero everything.
    *trj = (pbio_trajectory_t) {0};

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Set speeds, scaled to ddeg/s.
    trj->w0 = trj->w1 = trj->wu = to_trajectory_speed(c->speed_target);
    trj->w3 = c->continue_running ? to_trajectory_speed(c->speed_target): 0;
}

/**
 * Gets the traversed angle when accelerating from one speed value to another.
 *
 * Divides speed^2 by acceleration*2, giving angle.
 *
 * @param [in]  w_end   The ending speed in ddeg/s.
 * @param [in]  w_start The starting speed in ddeg/s.
 * @param [in]  a       The acceleration in deg/s^2.
 * @returns             The angle in mdeg.
 */
static int32_t div_w2_by_a(int32_t w_end, int32_t w_start, int32_t a) {

    assert_accel_numerator(a);
    assert_speed(w_end);
    assert_speed(w_start);

    return pbio_int_math_mult_then_div(w_end * w_end - w_start * w_start, (10 / 2), a);
}

/**
 * Divides speed by acceleration, giving time.
 *
 * @param [in]  w       The speed in ddeg/s.
 * @param [in]  a       The acceleration in deg/s^2.
 * @returns             The time in s*10^-4.
 */
static int32_t div_w_by_a(int32_t w, int32_t a) {

    assert_accel_numerator(a);
    assert_speed_rel(w);

    return w * 1000 / a;
}

/**
 * Divides angle by time, giving speed.
 *
 * @param [in]  th      The angle in mdeg.
 * @param [in]  t       The time in s*10^-4.
 * @returns             The speed in ddeg/s.
 */
static int32_t div_th_by_t(int32_t th, int32_t t) {

    assert_time(t);
    assert_angle(th);

    if (pbio_int_math_abs(th) < INT32_MAX / 100) {
        return th * 100 / t;
    }
    return th / t * 100;
}

/**
 * Divides speed by time, giving acceleration.
 *
 * @param [in]  w       The speed in ddeg/s.
 * @param [in]  t       The time in s*10^-4.
 * @returns             The acceleration in deg/s^2.
 */
static int32_t div_w_by_t(int32_t w, int32_t t) {

    assert_time(t);
    assert_speed_rel(w);

    return w * 1000 / t;
}

/**
 * Divides angle by speed, giving time (s*10^-4).
 *
 * @param [in]  th      The angle in mdeg.
 * @param [in]  w       The speed in ddeg/s.
 * @returns             The time in s*10^-4.
 */
static int32_t div_th_by_w(int32_t th, int32_t w) {

    assert_angle(th);
    assert_speed_rel(w);

    return pbio_int_math_mult_then_div(th, 100, w);
}

/**
 * Multiplies speed by time, giving angle.
 *
 * @param [in]  w       The speed in ddeg/s.
 * @param [in]  t       The time in s*10^-4.
 * @returns             The angle in mdeg.
 */
static int32_t mul_w_by_t(int32_t w, int32_t t) {

    assert_time(t);
    assert_speed_rel(w);

    return pbio_int_math_mult_then_div(w, t, 100);
}

/**
 * Multiplies acceleration by time, giving speed.
 *
 * @param [in]  a       The acceleration in deg/s^2.
 * @param [in]  t       The time in s*10^-4.
 * @returns             The speed in ddeg/s.
 */
static int32_t mul_a_by_t(int32_t a, int32_t t) {

    assert_time(t);
    assert_accel_small(a);
    assert_accel_time(t);

    return pbio_int_math_mult_then_div(a, t, 1000);
}

/**
 * Multiplies acceleration by time^2/2, giving angle.
 *
 * @param [in]  a       The acceleration in deg/s^2.
 * @param [in]  t       The time in s*10^-4.
 * @returns             The angle in mdeg.
 */
static int32_t mul_a_by_t2(int32_t a, int32_t t) {

    assert_time(t);
    assert_accel_small(a);
    assert_accel_time(t);

    return mul_w_by_t(mul_a_by_t(a, t), t) / 2;
}

/**
 * Gets starting speed to reach end speed within given angle and acceleration.
 *
 * Inverse of div_w2_by_a().
 *
 * @param [in]  w_end   The ending speed in ddeg/s.
 * @param [in]  a       The acceleration in deg/s^2.
 * @param [in]  th      The angle in mdeg.
 * @returns             The starting speed in ddeg/s.
 */
static int32_t bind_w0(int32_t w_end, int32_t a, int32_t th) {

    assert_accel_small(a);
    assert_speed(w_end);
    assert((int64_t)pbio_int_math_abs(a) * (int64_t)pbio_int_math_abs(th) < INT32_MAX);

    // This is only called if the acceleration is not high enough to reach
    // the end speed within a certain angle. So only if the angle is smaller
    // than w2 / a / 2, which is safe.
    return pbio_int_math_sqrt(w_end * w_end + a * th / (10 / 2));
}

/**
 * Gets the intersection of two speed curves.
 *
 * Given a stationary starting and final angle, computes the intersection of
 * the speed curves if we accelerate up and down without a stationary speed phase.
 *
 * @param [in]  th3     The final angle in mdeg.
 * @param [in]  th0     The starting angle in mdeg.
 * @param [in]  a0      The "up" acceleration in deg/s^2.
 * @param [in]  a2      The "down" acceleration in deg/s^2.
 * @returns             The angle at which the two curves intersect in mdeg.
 */
static int32_t intersect_ramp(int32_t th3, int32_t th0, int32_t a0, int32_t a2) {

    assert_accel_angle(th3 - th0);

    // If angles are equal, that's where they intersect.
    // This avoids the acceleration ratio division, which
    // is needed when a2 == a0, which is allowed in this
    // scenario. This happens when the movement consists
    // of just deceleration the whole way.
    if (th3 == th0) {
        return th0;
    }

    assert_accel_numerator(a0);
    assert_accel_numerator(a2);
    assert(a0 != a2);

    return th0 + pbio_int_math_mult_then_div(th3 - th0, a2, a2 - a0);
}

/**
 * Computes a trajectory for a timed command assuming *positive* speed.
 *
 * @param [out] trj     An uninitialized trajectory to hold the result.
 * @param [in]  c       The command to use.
 * @returns             ::PBIO_ERROR_INVALID_ARG if the command duration is out of range or the resulting angle is too large,
 *                      ::PBIO_ERROR_FAILED if any of the calculated intervals are non-positive,
 *                      otherwise ::PBIO_SUCCESS.
 */
static pbio_error_t pbio_trajectory_new_forward_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Return error for a long user-specified duration.
    if (c->duration > TIME_MAX) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Save duration.
    trj->t3 = TO_TRAJECTORY_TIME(c->duration);

    // Speed continues at target speed or goes to zero. And scale to ddeg/s.
    trj->w3 = c->continue_running ? to_trajectory_speed(c->speed_target) : 0;
    trj->w0 = to_trajectory_speed(c->speed_start);
    int32_t wt = (trj->wu = to_trajectory_speed(c->speed_target));
    int32_t accel = to_trajectory_accel(c->acceleration);
    int32_t decel = to_trajectory_accel(c->deceleration);

    // Return error if approximate angle too long.
    if (mul_w_by_t(wt, trj->t3) > ANGLE_MAX) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Bind initial speed to make solution feasible.
    if (div_w_by_a(trj->w0, accel) < -trj->t3) {
        trj->w0 = -mul_a_by_t(accel, trj->t3);
    }
    if (div_w_by_a(trj->w0 - trj->w3, pbio_int_math_max(accel, decel)) > trj->t3) {
        trj->w0 = trj->w3 + mul_a_by_t(pbio_int_math_max(accel, decel), trj->t3);
    }

    // Bind target speed to make solution feasible.
    if (div_w_by_a(wt - trj->w3, decel) > trj->t3) {
        wt = trj->w3 + mul_a_by_t(decel, trj->t3);
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsically dealt with in the
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
            trj->w1 = trj->w0 + mul_a_by_t(trj->a0, trj->t3);
            trj->w3 = trj->w1;
        } else {
            // Otherwise, we can just take the intersection of the accelerating
            // and decelerating ramps to find the angle at t1 = t2.
            assert(trj->a0 != trj->a2);
            trj->t1 = div_w_by_a(trj->w3 - trj->w0 - mul_a_by_t(trj->a2, trj->t3), trj->a0 - trj->a2);

            // There is no constant speed phase in this case.
            t2mt1 = 0;
            trj->w1 = trj->w0 + mul_a_by_t(trj->a0, trj->t1);
        }
    }

    // For numerically negligible speed differences (achieved in zero time),
    // set the initial speed equal to speed at t1. This ensures correct
    // behavior when evaluating the reference when time is 0.
    if (trj->t1 == 0) {
        trj->w0 = trj->w1;
    }

    // With the difference between t1 and t2 known, we know t2.
    trj->t2 = trj->t1 + t2mt1;

    // Corresponding angle values
    trj->th1 = div_w2_by_a(trj->w1, trj->w0, trj->a0);
    trj->th2 = trj->th1 + mul_w_by_t(trj->w1, t2mt1);
    trj->th3 = trj->th2 + div_w2_by_a(trj->w3, trj->w1, trj->a2);

    // Assert that all resulting time intervals are positive.
    if (trj->t1 < 0 || trj->t2 - trj->t1 < 0 || trj->t3 - trj->t2 < 0) {
        return PBIO_ERROR_FAILED;
    }
    return PBIO_SUCCESS;
}

/**
 * Computes a trajectory for an angle command assuming *positive* speed.
 *
 * @param [out] trj     An uninitialized trajectory to hold the result.
 * @param [in]  c       The command to use.
 * @returns             ::PBIO_ERROR_INVALID_ARG if the command duration is out of range,
 *                      ::PBIO_ERROR_FAILED if any of the calculated intervals are non-positive,
 *                      otherwise ::PBIO_SUCCESS.
 */
static pbio_error_t pbio_trajectory_new_forward_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *c) {

    // Fill out starting point based on user command.
    pbio_trajectory_set_start(&trj->start, c);

    // Get angle to travel.
    trj->th3 = pbio_angle_diff_mdeg(&c->position_end, &c->position_start);

    // Speed continues at target speed or goes to zero. And scale to ddeg/s.
    trj->w3 = c->continue_running ? to_trajectory_speed(c->speed_target) : 0;
    trj->w0 = to_trajectory_speed(c->speed_start);
    int32_t wt = (trj->wu = to_trajectory_speed(c->speed_target));
    int32_t accel = to_trajectory_accel(c->acceleration);
    int32_t decel = to_trajectory_accel(c->deceleration);

    // Bind initial speed to make solution feasible. Do the larger-than check
    // using quadratic terms to avoid square root evaluations in most cases.
    // This is only needed for positive initial speed, because negative initial
    // speeds are always feasible in angle-based maneuvers.
    int32_t a_max = pbio_int_math_max(accel, decel);
    if (trj->w0 > 0 && div_w2_by_a(trj->w0, trj->w3, a_max) > trj->th3) {
        trj->w0 = bind_w0(trj->w3, a_max, trj->th3);
    }

    // Do the same check for the target speed, but now use the deceleration
    // magnitude as the only constraint. The target speed is always positive,
    // so we can omit that additional check here. We have the full angle (th3)
    // to slow down, plus any angle run backwards due to initial speed, if any.
    int32_t fwd_angle = trj->th3 - (trj->w0 > 0 ? 0 : div_w2_by_a(0, trj->w0, accel));
    if (div_w2_by_a(wt, trj->w3, decel) > fwd_angle) {
        wt = bind_w0(trj->w3, decel, fwd_angle);
    }

    // Get initial approximation of duration.
    int32_t t3_approx = div_th_by_w(fwd_angle, wt);
    if (trj->w0 < 0) {
        // Add time to dissipate initial speed. The case of forward
        // excess speed gets handled below by cutting down speed.
        t3_approx += div_w_by_a(-trj->w0, accel);
    }
    // Return error if maneuver would take too long.
    if (t3_approx > TIME_MAX) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Initial acceleration sign depends on initial speed. It accelerates if
    // the initial speed is less than the target speed. Otherwise it
    // decelerates. The equality case is intrinsically dealt with in the
    // nominal acceleration case down below.
    trj->a0 = trj->w0 < wt ? accel : -accel;

    // Since our maneuver is forward, final deceleration is always negative.
    trj->a2 = -decel;

    // Now we can evaluate the nominal cases and check if they hold. First get
    // a fictitious zero-speed angle for computational convenience. This is the
    // angle on the speed-angle phase plot where the in-phase square root
    // function intersects the zero speed axis if we kept (de)accelerating.
    int32_t thf = div_w2_by_a(0, trj->w0, trj->a0);

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
        } else if (c->continue_running && trj->a0 < 0) {
            // If we have a nonzero final speed and initial negative
            // acceleration, we are going too fast and we would overshoot
            // the target before having slowed down. So we need to reduce the
            // initial speed to be able to reach it.
            trj->w0 = bind_w0(trj->w3, -trj->a0, trj->th3);
            trj->w1 = trj->w3;
            trj->th1 = trj->th3;
            trj->th2 = trj->th3;
        } else {
            // Otherwise we have a zero final speed. We can just take the
            // intersection of the accelerating and decelerating ramps to find
            // the speed at t1 = t2.
            trj->th1 = intersect_ramp(trj->th3, thf, trj->a0, trj->a2);
            trj->th2 = trj->th1;
            trj->w1 = bind_w0(0, trj->a0, trj->th1 - thf);

            // If w0 and w1 are very close, the previously determined
            // acceleration sign may be wrong after rounding errors, so update.
            trj->a0 = trj->w0 < trj->w1 ? accel : -accel;
        }
    }

    // Constant speed phase duration, if any.
    int32_t t2mt1 = trj->th2 == trj->th1 ? 0 : div_th_by_w(trj->th2 - trj->th1, trj->w1);

    // With the intermediate angles and speeds now known, we can calculate the
    // corresponding durations to match.
    trj->t1 = div_w_by_a(trj->w1 - trj->w0, trj->a0);

    // For numerically negligible speed differences (achieved in zero time),
    // set the initial speed equal to speed at t1. This ensures correct
    // behavior when evaluating the reference when time is 0.
    if (trj->t1 == 0) {
        trj->w0 = trj->w1;
    }

    trj->t2 = trj->t1 + t2mt1;
    trj->t3 = trj->t2 + div_w_by_a(trj->w3 - trj->w1, trj->a2);

    // Ensure computed motion points are ordered.
    if (trj->th2 < trj->th1 || trj->th3 < trj->th2) {
        return PBIO_ERROR_FAILED;
    }

    // Assert times are valid.
    assert_time(trj->t1);
    assert_time(trj->t2);
    assert_time(trj->t3);
    assert_time(trj->t2 - trj->t1);
    assert_time(trj->t3 - trj->t2);
    if (trj->t1 < 0 || trj->t2 - trj->t1 < 0 || trj->t3 - trj->t2 < 0) {
        return PBIO_ERROR_FAILED;
    }
    return PBIO_SUCCESS;
}

/**
 * Stretches a trajectory to end at the same time as @p leader.
 *
 * @param [in]  trj     An initalized trajectory to be modified.
 * @param [in]  leader  The trajectory to follow.
 */
void pbio_trajectory_stretch(pbio_trajectory_t *trj, const pbio_trajectory_t *leader) {

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

/**
 * Computes a trajectory for a timed command.
 *
 * @param [out] trj     An uninitialized trajectory to hold the result.
 * @param [in]  command The command to use.
 * @returns             ::PBIO_ERROR_INVALID_ARG if the command duration is out of range or the resulting angle is too large,
 *                      ::PBIO_ERROR_FAILED if any of the calculated intervals are non-positive,
 *                      otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_trajectory_new_time_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command) {

    // Copy the command so we can modify it.
    pbio_trajectory_command_t c = *command;

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
    c.speed_target = pbio_int_math_min(c.speed_target, c.speed_max);

    // Calculate the trajectory, assumed to be forward.
    pbio_error_t err = pbio_trajectory_new_forward_time_command(trj, &c);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }
    return PBIO_SUCCESS;
}

/**
 * Computes a trajectory for an angle command.
 *
 * @param [out] trj     An uninitialized trajectory to hold the result.
 * @param [in]  command The command to use.
 * @returns             ::PBIO_ERROR_INVALID_ARG if the command duration is out of range or the angle is too large,
 *                      ::PBIO_ERROR_FAILED if any of the calculated intervals are non-positive,
 *                      otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbio_trajectory_new_angle_command(pbio_trajectory_t *trj, const pbio_trajectory_command_t *command) {

    // Copy the command so we can modify it.
    pbio_trajectory_command_t c = *command;

    // Return error for maneuver that is too long by angle.
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

    // Direction is solely defined in terms of sign of the angle th3.
    // For speed, only the *magnitude* is relevant. Certain end-user APIs
    // allow specifying physically impossible scenarios like negative speed
    // with a positive relative position. Those cases are not handled here and
    // should be appropriately handled at higher levels.
    c.speed_target = pbio_int_math_abs(c.speed_target);

    // Bind target speed by maximum speed.
    c.speed_target = pbio_int_math_min(c.speed_target, c.speed_max);

    // Check if the original user-specified maneuver is backward.
    bool backward = distance < 0;

    // Convert user command into a forward maneuver to simplify computations.
    if (backward) {
        c.position_end = c.position_start;
        pbio_angle_add_mdeg(&c.position_end, -distance);
        c.speed_start *= -1;
    }

    // Calculate the trajectory, assumed to be forward.
    pbio_error_t err = pbio_trajectory_new_forward_angle_command(trj, &c);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reverse the maneuver if the original arguments imposed backward motion.
    if (backward) {
        reverse_trajectory(trj);
    }

    return PBIO_SUCCESS;
}

/**
 * Populates reference point with the right units and offset.
 *
 * @param [out] ref     An uninitialized trajectory reference point to hold the result.
 * @param [in]  start   The starting trajectory reference point.
 * @param [in]  t       The time in s*10^-4.
 * @param [in]  th      The angle in mdeg.
 * @param [in]  w       The rotational speed in ddeg/s.
 * @param [in]  a       The acceleration in deg/s^2.
 */
static void pbio_trajectory_offset_start(pbio_trajectory_reference_t *ref, const pbio_trajectory_reference_t *start, int32_t t, int32_t th, int32_t w, int32_t a) {

    // Convert local trajectory units to global pbio units.
    ref->time = TO_CONTROL_TIME(t) + start->time;
    ref->speed = to_control_speed(w);
    ref->acceleration = to_control_accel(a);

    // Reference position is starting point plus progress in this maneuver.
    ref->position = start->position;
    pbio_angle_add_mdeg(&ref->position, th);
}

/**
 * Gets the last vertex of a trajectory before a given point in time.
 *
 * @param [in]  trj         The trajectory instance.
 * @param [in]  time_ref    The duration of time after the start of the trajectory in s*10^-4.
 * @param [out] vertex      An uninitialized trajectory reference point to hold the result.
 */
void pbio_trajectory_get_last_vertex(const pbio_trajectory_t *trj, uint32_t time_ref, pbio_trajectory_reference_t *vertex) {

    // Relative time within ongoing maneuver.
    int32_t time = TO_TRAJECTORY_TIME(time_ref - trj->start.time);
    assert_time(time);

    // Find which section of the ongoing maneuver we were in, and take
    // corresponding segment starting point. Acceleration is undefined but not
    // used when synchronizing trajectories, so set to zero.
    if (time - trj->t1 < 0 || (trj->t1 == 0 && time == 0)) {
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

/**
 * Gets the trajectory endpoint.
 *
 * @param [in]  trj         The trajectory instance.
 * @param [out] end         An uninitialized trajectory reference point to hold the result.
 */
void pbio_trajectory_get_endpoint(const pbio_trajectory_t *trj, pbio_trajectory_reference_t *end) {
    pbio_trajectory_offset_start(end, &trj->start, trj->t3, trj->th3, trj->w3, 0);
}

/**
 * Gets the originally commanded speed for the current trajectory, even if
 * this top speed is not reached.
 *
 * This is an indicator of the expected speed, which may be used to to alter
 * control behavior for motion with sustained low speed values.
 *
 * @param [in]  trj         The trajectory instance.
 * @return                  The user given speed in control units (mdeg/s).
 */
int32_t pbio_trajectory_get_abs_command_speed(const pbio_trajectory_t *trj) {
    return to_control_speed(pbio_int_math_abs(trj->wu));
}

/**
 * Gets the trajectory duration.
 *
 * The duration is the time at when the endpoint is expected to occur.
 *
 * @param [in]  trj         The trajectory instance.
 * @return                  The duration of time after the start of the trajectory in s*10^-4.
 */
uint32_t pbio_trajectory_get_duration(const pbio_trajectory_t *trj) {
    assert_time(trj->t3);
    return TO_CONTROL_TIME(trj->t3);
}

/**
 * Gets the calculated reference speed and velocity of the trajectory at the (shifted) time.
 *
 * @param [in]  trj         The trajectory instance.
 * @param [in]  time_ref    The duration of time after the start of the trajectory in s*10^-4.
 * @param [out] ref         An uninitialized trajectory reference point to hold the result.
 */
void pbio_trajectory_get_reference(pbio_trajectory_t *trj, uint32_t time_ref, pbio_trajectory_reference_t *ref) {

    // Time within maneuver since start.
    int32_t time = TO_TRAJECTORY_TIME(time_ref - trj->start.time);
    assert_time(time);

    // Get angle, speed, and acceleration along reference
    int32_t th;
    int32_t w;
    int32_t a;

    if (time - trj->t1 < 0 || (trj->t1 == 0 && time == 0)) {
        // If we are here, then we are still in the acceleration phase.
        // Includes conversion from microseconds to seconds, in two steps to
        // avoid overflows and round off errors
        w = trj->w0 + mul_a_by_t(trj->a0, time);
        th = mul_w_by_t(trj->w0, time) + mul_a_by_t2(trj->a0, time);
        a = trj->a0;
    } else if (time - trj->t2 < 0) {
        // If we are here, then we are in the constant speed phase
        w = trj->w1;
        th = trj->th1 + mul_w_by_t(trj->w1, time - trj->t1);
        a = 0;
    } else if (time - trj->t3 < 0) {
        // If we are here, then we are in the deceleration phase
        w = trj->w1 + mul_a_by_t(trj->a2, time - trj->t2);
        th = trj->th2 + mul_w_by_t(trj->w1, time - trj->t2) + mul_a_by_t2(trj->a2, time - trj->t2);
        a = trj->a2;
    } else {
        // If we are here, we are in the constant speed phase after the
        // maneuver completes
        w = trj->w3;
        th = trj->th3 + mul_w_by_t(trj->w3, time - trj->t3);
        a = 0;

        // To avoid any overflows of the aforementioned time comparisons,
        // rebase the trajectory if it has been running a long time.
        if (time > PBIO_TRAJECTORY_DURATION_FOREVER_MS * PBIO_TRAJECTORY_TICKS_PER_MS) {
            pbio_angle_t start = trj->start.position;
            pbio_angle_add_mdeg(&start, th);

            pbio_trajectory_command_t command = {
                .time_start = time_ref,
                .speed_target = to_control_speed(trj->w3),
                .continue_running = true,
                .position_start = start,
            };
            pbio_trajectory_make_constant(trj, &command);

            // w, and a are already set above. Time and angle are 0, since this
            // is the start of the new maneuver with its new starting point.
            time = 0;
            th = 0;
        }
    }

    // Assert that results are bounded
    assert_time(time);
    assert_angle(th);
    assert_speed(w);

    // Convert back to absolute points by adding starting point.
    pbio_trajectory_offset_start(ref, &trj->start, time, th, w, a);
}
