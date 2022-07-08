// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdio.h>

#include <pbio/trajectory.h>
#include <pbio/util.h>

#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#define MDEG_PER_DEG (1000)
#define TICKS_PER_MS (10)
#define DDEGS_PER_DEGS (10)

static pbio_angle_t origin = {
    .rotations = 0,
    .millidegrees = 0,
};

static void test_simple_trajectory(void *env) {

    // Command: Run for 10000 degrees at 1000 deg/s with a = 2000 deg/s/s.
    // Ramping up and down takes 500 ms this way, during which we travel 250
    // degrees, so overal expected duration is 10500 ms.

    pbio_angle_t end = {
        .rotations = 27,
        .millidegrees = 280 * MDEG_PER_DEG,
    };

    pbio_trajectory_command_t command = {
        .time_start = 0,
        .position_start = origin,
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

    tt_want_int_op(trj.t1, ==, 500 * TICKS_PER_MS);
    tt_want_int_op(trj.t2, ==, 10000 * TICKS_PER_MS);
    tt_want_int_op(trj.t3, ==, 10500 * TICKS_PER_MS);
    tt_want_int_op(trj.th1, ==, 250 * MDEG_PER_DEG);
    tt_want_int_op(trj.th2, ==, 9750 * MDEG_PER_DEG);
    tt_want_int_op(trj.th3, ==, 10000 * MDEG_PER_DEG);
    tt_want_int_op(trj.w0, ==, 0);
    tt_want_int_op(trj.w1 / DDEGS_PER_DEGS, ==, command.speed_target / MDEG_PER_DEG);
    tt_want_int_op(trj.w3 / DDEGS_PER_DEGS, ==, 0);
    tt_want_int_op(trj.a0, ==, command.acceleration / MDEG_PER_DEG);
    tt_want_int_op(trj.a2, ==, -command.deceleration / MDEG_PER_DEG);
}

static void test_infinite_trajectory(void *env) {

    int32_t accelerations[] = {
        10, 50, 100, 500, 1000, 2000, 5000, 10000, 20000, 40000
    };

    int32_t speeds[] = {
        -2000, -1000, -500, -10, 0, 10, 500, 1000, 2000,
    };

    int32_t times[] = {
        INT32_MIN, INT32_MIN / 2, -1, 0, 1, INT32_MAX / 2, INT32_MAX
    };

    // Go through range of accelerations.
    for (int a = 0; a < PBIO_ARRAY_SIZE(accelerations); a++) {
        // Go through range of target speeds.
        for (int wt = 0; wt < PBIO_ARRAY_SIZE(speeds); wt++) {
            // Go through range of initial speeds.
            for (int w0 = 0; w0 < PBIO_ARRAY_SIZE(speeds); w0++) {
                // Go through range of initial times.
                for (int t = 0; t < PBIO_ARRAY_SIZE(times); t++) {
                    // Define the command for this permutation of parameters.
                    pbio_trajectory_command_t command = {
                        .time_start = times[t],
                        .duration = DURATION_FOREVER_TICKS,
                        .position_start = origin,
                        .position_end = origin,
                        .speed_start = speeds[w0] * DDEGS_PER_DEGS,
                        .speed_target = speeds[wt] * DDEGS_PER_DEGS,
                        .speed_max = 1000 * DDEGS_PER_DEGS,
                        .acceleration = accelerations[a] * MDEG_PER_DEG,
                        .deceleration = accelerations[a] * MDEG_PER_DEG,
                        .continue_running = true,
                    };

                    // Calculate the trajectory.
                    pbio_trajectory_t trj;
                    pbio_error_t err = pbio_trajectory_new_time_command(&trj, &command);
                    tt_want_int_op(err, ==, PBIO_SUCCESS);

                    // Verify that we maintain a constant speed when done.
                    tt_want_int_op(trj.w1, ==, trj.w3);

                    // Initial speed may now be bounded. Verify that the sign
                    // is the same and that it did not grow in size.
                    if (command.speed_start > 0) {
                        tt_want_int_op(trj.w0 / DDEGS_PER_DEGS, <=, command.speed_start / MDEG_PER_DEG);
                    }
                    if (command.speed_start < 0) {
                        tt_want_int_op(trj.w0 / DDEGS_PER_DEGS, >=, command.speed_start / MDEG_PER_DEG);
                    } else {
                        tt_want_int_op(trj.w0 / DDEGS_PER_DEGS, ==, command.speed_start / MDEG_PER_DEG);
                    }

                    // Verify that the target speed is reached, which should
                    // always be the case in an infinite maneuver.
                    if (command.speed_target > command.speed_max) {
                        tt_want_int_op(trj.w1 / DDEGS_PER_DEGS, ==, command.speed_max / MDEG_PER_DEG);
                    } else if (command.speed_target < -command.speed_max) {
                        tt_want_int_op(trj.w1 / DDEGS_PER_DEGS, ==, -command.speed_max / MDEG_PER_DEG);
                    } else {
                        tt_want_int_op(trj.w1 / DDEGS_PER_DEGS, ==, command.speed_target / MDEG_PER_DEG);
                    }
                }
            }
        }
    }
}

struct testcase_t pbio_trajectory_tests[] = {
    PBIO_TEST(test_simple_trajectory),
    PBIO_TEST(test_infinite_trajectory),
    END_OF_TESTCASES
};
