// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdio.h>
#include <stdlib.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/util.h>

#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#define MDEG_PER_DEG (1000)

/**
 * This tests one trajectory with simple numbers. If there is a regression,
 * this one is easy to debug.
 */
static void test_simple_trajectory(void *env) {

    // Command: Run for 10000 degrees at 1000 deg/s with a = 2000 deg/s/s.
    // Ramping up and down takes 500 ms this way, during which we travel 250
    // degrees, so overal expected duration is 10500 ms.

    static pbio_angle_t start = {
        .rotations = 0,
        .millidegrees = 0,
    };

    pbio_angle_t end = {
        .rotations = 27,
        .millidegrees = 280 * MDEG_PER_DEG,
    };

    pbio_trajectory_command_t command = {
        .time_start = 0,
        .position_start = start,
        .position_end = end,
        .speed_start = 0,
        .speed_target = 1000 * MDEG_PER_DEG,
        .speed_max = 1000 * MDEG_PER_DEG,
        .acceleration = 2000 * MDEG_PER_DEG,
        .deceleration = 2000 * MDEG_PER_DEG,
        .continue_running = false,
    };

    pbio_trajectory_t trj;
    pbio_error_t err = pbio_trajectory_new_angle_command(&trj, &command);
    tt_want_int_op(err, ==, PBIO_SUCCESS);

    tt_want_int_op(trj.t1, ==, 500 * 10);
    tt_want_int_op(trj.t2, ==, 10000 * 10);
    tt_want_int_op(trj.t3, ==, 10500 * 10);
    tt_want_int_op(trj.th1, ==, 250 * MDEG_PER_DEG);
    tt_want_int_op(trj.th2, ==, 9750 * MDEG_PER_DEG);
    tt_want_int_op(trj.th3, ==, 10000 * MDEG_PER_DEG);
    tt_want_int_op(trj.w0, ==, 0);
    tt_want_int_op(trj.w1, ==, command.speed_target / 100);
    tt_want_int_op(trj.w3, ==, 0);
    tt_want_int_op(trj.a0, ==, command.acceleration / MDEG_PER_DEG);
    tt_want_int_op(trj.a2, ==, -command.deceleration / MDEG_PER_DEG);
}

static void walk_trajectory(pbio_trajectory_t *trj) {

    // Get the starting reference.
    pbio_trajectory_reference_t ref_prev, ref_now;
    pbio_trajectory_get_reference(trj, trj->start.time, &ref_prev);

    uint32_t time_start = ref_prev.time;

    // Get duration of trajectory.
    uint32_t duration = pbio_trajectory_get_duration(trj);

    if (duration == DURATION_FOREVER_TICKS) {
        // To save time on testing, check only twice the on-ramp for "forever"
        // maneuvers. The lengthy constant speed phase is checked separately
        // later on anyway.
        duration = trj->t1 * 2;
    } else {
        // For standard maneuvers, check slightly more than the length, to
        // include testing the behavior after completion. Either add a second,
        // or do twice the duration, whichever is less.
        duration = pbio_math_min(duration + 10000, duration * 2);
    }

    // Loop over all trajectory points and assert results.
    for (uint32_t t = 1; t < duration; t += 500) {

        // Get current reference.
        uint32_t now = t + time_start;
        pbio_trajectory_get_reference(trj, now, &ref_now);

        // Current time should match
        tt_want_int_op(ref_now.time, ==, now);

        bool same_speed_dir = pbio_math_sign(ref_now.speed) == pbio_math_sign(ref_prev.speed);
        bool same_accel_dir = pbio_math_sign(ref_now.acceleration) == pbio_math_sign(ref_prev.acceleration);

        // If the speed and acceleration direction was the same between two
        // samples, we can test that the position increment is correct.
        if (same_speed_dir && same_accel_dir) {

            // Position increment should match speed direction.
            int32_t movement = pbio_angle_diff_mdeg(&ref_now.position, &ref_prev.position);
            tt_want(pbio_math_sign_not_opposite(ref_prev.speed, movement / 1000));

            // Speed increment should match acceleration direction.
            tt_want(pbio_math_sign_not_opposite(ref_now.speed - ref_prev.speed, ref_prev.acceleration));
        }

        // Set reference for next comparison.
        ref_prev = ref_now;
        ref_prev.position = ref_now.position;
    }
}

// Start and end angles in millidegrees.
static const pbio_angle_t angles[] = {
    {.rotations = 0,             .millidegrees = 0 },
    {.rotations = 0,             .millidegrees = 1 * MDEG_PER_DEG },
    {.rotations = 0,             .millidegrees = 30 * MDEG_PER_DEG },
    {.rotations = 0,             .millidegrees = 360 * MDEG_PER_DEG },
    {.rotations = -4,            .millidegrees = 0},
    {.rotations = -123,          .millidegrees = 456 },
    {.rotations = INT32_MAX / 4, .millidegrees = INT32_MAX / 4 },
};

// Acceleration and deceleration test range in deg/s^2.
static const int32_t accelerations[] = {
    50, 500, 1000, 5000, 20000,
};

// Start and target speed range deg/s.
static const int32_t speeds[] = {
    -2000, -500, 0, 1, 10, 2000,
};

// Start time in clock ticks.
static const uint32_t start_times[] = {
    0, UINT32_MAX - 10000, UINT32_MAX,
};

// Number of permutations for infinite trajectories.
static const uint32_t num_infinite_trajectories =
    PBIO_ARRAY_SIZE(angles) * // For starting angle. End angle unused.
    PBIO_ARRAY_SIZE(accelerations) * // For acceleration. Deceleration unused.
    PBIO_ARRAY_SIZE(speeds) * // For start speed.
    PBIO_ARRAY_SIZE(speeds) * // For target speed.
    PBIO_ARRAY_SIZE(start_times);

/**
 * Get one command for testing an infinite trajectory.
 */
static void get_infinite_command(uint32_t index, pbio_trajectory_command_t *c) {

    c->duration = DURATION_FOREVER_TICKS;
    c->speed_max = 1000 * MDEG_PER_DEG;
    c->continue_running = true;

    c->position_start = angles[index % PBIO_ARRAY_SIZE(angles)];
    index /= PBIO_ARRAY_SIZE(angles);

    c->time_start = start_times[index % PBIO_ARRAY_SIZE(start_times)];
    index /= PBIO_ARRAY_SIZE(start_times);

    c->acceleration = accelerations[index % PBIO_ARRAY_SIZE(accelerations)] * MDEG_PER_DEG;
    c->deceleration = c->acceleration;
    index /= PBIO_ARRAY_SIZE(accelerations);

    c->speed_start = speeds[index % PBIO_ARRAY_SIZE(speeds)] * MDEG_PER_DEG;
    index /= PBIO_ARRAY_SIZE(speeds);

    c->speed_target = speeds[index % PBIO_ARRAY_SIZE(speeds)] * MDEG_PER_DEG;
    index /= PBIO_ARRAY_SIZE(speeds);
}

static void test_infinite_trajectory(void *env) {

    pbio_trajectory_command_t command;

    for (uint32_t i = 0; i < num_infinite_trajectories; i++) {
        get_infinite_command(i, &command);

        // Calculate the trajectory.
        pbio_trajectory_t trj;
        pbio_error_t err = pbio_trajectory_new_time_command(&trj, &command);
        tt_want_int_op(err, ==, PBIO_SUCCESS);

        // Verify that we maintain a constant speed when done.
        tt_want_int_op(trj.w1, ==, trj.w3);

        // Evaluate reference at several point to verify result.
        pbio_trajectory_reference_t ref;

        // Initial speed may now be bounded. Verify that the sign is the same.
        pbio_trajectory_get_reference(&trj, command.time_start, &ref);
        tt_want_int_op(ref.speed < 0, ==, command.speed_start < 0);

        // In the normal circumstance, the very first reference speed is not
        // bigger than the start speed of the command. But this only applies if
        // there is an acceleration phase at all. If the acceleration was very
        // high then the speed jumps to the target right away.
        if (trj.t1 != 0) {
            tt_want_int_op(pbio_math_abs(ref.speed), <=, pbio_math_abs(command.speed_start));
        }

        // Verify that the target speed is reached, which should
        // always be the case in an infinite maneuver, unless capped.
        int32_t expected_speed = pbio_math_abs(command.speed_target) < command.speed_max ?
            command.speed_target :
            pbio_math_sign(command.speed_target) * command.speed_max;

        // Walk the whole trajectory.
        walk_trajectory(&trj);

        // Walking the trajectory past "infinity" will rebase it automatically,
        // so compute it again so we can do additional checks for big time
        // values.
        err = pbio_trajectory_new_time_command(&trj, &command);
        tt_want_int_op(err, ==, PBIO_SUCCESS);

        // Verify that we keep hitting the expected speed.
        for (uint32_t i = 0; i < 10; i++) {
            pbio_trajectory_get_reference(&trj, command.time_start + trj.t1 + i * DURATION_FOREVER_TICKS / 4, &ref);
            tt_want_int_op(ref.speed, ==, expected_speed);
        }

        // Polling the inifinite trajectory for so long should by now have
        // rebased the trajectory to a new, constant command.
        tt_want(trj.t1 == 0 && trj.t2 == 0 && trj.t3 == 0);
        tt_want(trj.th1 == 0 && trj.th2 == 0 && trj.th3 == 0);
        tt_want(trj.a0 == 0 && trj.a2 == 0);
        tt_want(trj.w0 == trj.w1 && trj.w1 == trj.w3);
    }
}

// Number of permutations for position trajectories.
static const uint32_t num_position_trajectories =
    PBIO_ARRAY_SIZE(angles) * // For starting angle.
    PBIO_ARRAY_SIZE(angles) * // For final angle.
    PBIO_ARRAY_SIZE(accelerations) * // For acceleration.
    PBIO_ARRAY_SIZE(accelerations) * // For decceleration.
    PBIO_ARRAY_SIZE(speeds) * // For start speed.
    PBIO_ARRAY_SIZE(speeds) * // For target speed.
    PBIO_ARRAY_SIZE(start_times) * // For start time.
    2; // For do or don't continue running.


/**
 * Get one command for testing a position trajectory.
 */
static void get_position_command(uint32_t index, pbio_trajectory_command_t *c) {

    c->speed_max = 1000 * MDEG_PER_DEG;

    c->continue_running = index % 2;
    index /= 2;

    c->position_start = angles[index % PBIO_ARRAY_SIZE(angles)];
    index /= PBIO_ARRAY_SIZE(angles);

    c->position_end = angles[index % PBIO_ARRAY_SIZE(angles)];
    index /= PBIO_ARRAY_SIZE(angles);

    c->time_start = start_times[index % PBIO_ARRAY_SIZE(start_times)];
    index /= PBIO_ARRAY_SIZE(start_times);

    c->acceleration = accelerations[index % PBIO_ARRAY_SIZE(accelerations)] * MDEG_PER_DEG;
    index /= PBIO_ARRAY_SIZE(accelerations);

    c->deceleration = accelerations[index % PBIO_ARRAY_SIZE(accelerations)] * MDEG_PER_DEG;
    index /= PBIO_ARRAY_SIZE(accelerations);

    c->speed_start = speeds[index % PBIO_ARRAY_SIZE(speeds)] * MDEG_PER_DEG;
    index /= PBIO_ARRAY_SIZE(speeds);

    c->speed_target = speeds[index % PBIO_ARRAY_SIZE(speeds)] * MDEG_PER_DEG;
    index /= PBIO_ARRAY_SIZE(speeds);
}

static void test_position_trajectory(void *env) {

    pbio_trajectory_command_t command;

    for (uint32_t i = 0; i < num_position_trajectories; i++) {
        get_position_command(i, &command);

        // Calculate the trajectory.
        pbio_trajectory_t trj;
        pbio_error_t err = pbio_trajectory_new_angle_command(&trj, &command);

        // Very low speeds or accelerations with long angles are not valid.
        if (err == PBIO_ERROR_INVALID_ARG) {
            continue;
        }

        // Otherwise we want success.
        tt_want_int_op(err, ==, PBIO_SUCCESS);

        // Verify that we maintain a constant speed when done.
        if (command.continue_running) {
            tt_want_int_op(trj.w3, ==, trj.w1);
        } else {
            tt_want_int_op(trj.w3, ==, 0);
        }

        // Initial speed may now be bounded. Verify that the sign is the same.
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&trj, command.time_start, &ref);
        get_position_command(i, &command);
        tt_want(pbio_math_sign_not_opposite(ref.speed, command.speed_start));

        // Walk the whole trajectory.
        walk_trajectory(&trj);
    }
}

struct testcase_t pbio_trajectory_tests[] = {
    PBIO_TEST(test_simple_trajectory),
    PBIO_TEST(test_position_trajectory),
    PBIO_TEST(test_infinite_trajectory),
    END_OF_TESTCASES
};
