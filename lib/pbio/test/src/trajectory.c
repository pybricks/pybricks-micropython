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

// Start and end angles in millidegrees.
static const pbio_angle_t angles[] = {
    {.rotations = 0,             .millidegrees = 0 },
    {.rotations = -123,          .millidegrees = 456 },
    {.rotations = 0,             .millidegrees = INT32_MAX / 4 },
    {.rotations = INT32_MAX / 4, .millidegrees = 0 },
};

// Acceleration and deceleration test range in deg/s^2.
static const int32_t accelerations[] = {
    10, 50, 100, 500, 1000, 2000, 5000, 10000, 20000, 40000,
};

// Start and target speed range deg/s.
static const int32_t speeds[] = {
    -2000, -500, 0, 1, 10, 500, 1000, 2000,
};

// Start time in clock ticks.
static const int32_t start_times[] = {
    INT32_MIN, INT32_MIN / 2, -1, 0, 1, INT32_MAX / 2, INT32_MAX
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

        // Verify that we keep hitting the expected speed.
        for (uint32_t i = 0; i < 10; i++) {
            pbio_trajectory_get_reference(&trj, command.time_start + trj.t1 + i * DURATION_FOREVER_TICKS / 4, &ref);
            tt_want_int_op(ref.speed, ==, expected_speed);
        }
    }
}

struct testcase_t pbio_trajectory_tests[] = {
    PBIO_TEST(test_simple_trajectory),
    PBIO_TEST(test_infinite_trajectory),
    END_OF_TESTCASES
};
