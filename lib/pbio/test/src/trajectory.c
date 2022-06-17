// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdio.h>

#include <pbio/trajectory.h>
#include <pbio/util.h>

#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

static void test_simple_trajectory(void *env) {

    // Command: Run for 10000 degrees at 1000 deg/s with a = 2000 deg/s/s.
    // Ramping up and down takes 500 ms this way, during which we travel 250
    // degrees, so overal expected duration is 10500 ms.
    pbio_trajectory_command_t command = {
        .time_start = 0,
        .angle_start = 0,
        .angle_start_ext = 0,
        .angle_end = 10000,
        .speed_start = 0,
        .speed_target = 1000,
        .speed_max = 1000,
        .acceleration = 2000,
        .deceleration = 2000,
        .continue_running = false,
    };

    pbio_trajectory_t trj;
    pbio_error_t err = pbio_trajectory_new_angle_command(&trj, &command);
    tt_want_int_op(err, ==, PBIO_SUCCESS);

    tt_want_int_op(trj.t1, ==, 500 * US_PER_MS);
    tt_want_int_op(trj.t2, ==, 10000 * US_PER_MS);
    tt_want_int_op(trj.t3, ==, 10500 * US_PER_MS);
    tt_want_int_op(trj.th1, ==, 250);
    tt_want_int_op(trj.th2, ==, 9750);
    tt_want_int_op(trj.th3, ==, command.angle_end);
    tt_want_int_op(trj.th1_ext, ==, 0);
    tt_want_int_op(trj.th2_ext, ==, 0);
    tt_want_int_op(trj.th3_ext, ==, 0);
    tt_want_int_op(trj.w0, ==, 0);
    tt_want_int_op(trj.w1, ==, command.speed_target);
    tt_want_int_op(trj.w3, ==, 0);
    tt_want_int_op(trj.a0, ==, command.acceleration);
    tt_want_int_op(trj.a2, ==, -command.deceleration);
}

static void test_infinite_trajectory(void *env) {

    int32_t accelerations[] = {
        10, 50, 100, 500, 1000, 2000, 5000, 10000, 20000
    };

    int32_t speeds[] = {
        // TODO: Fix failure on excessive speeds like 10000+
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
                        .duration = DURATION_FOREVER_MS * US_PER_MS,
                        .angle_start = 0,
                        .angle_start_ext = 0,
                        .speed_start = speeds[w0],
                        .speed_target = speeds[wt],
                        .speed_max = 1000,
                        .acceleration = accelerations[a],
                        .deceleration = accelerations[a],
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
                        tt_want_int_op(trj.w0, <=, command.speed_start);
                    }
                    if (command.speed_start < 0) {
                        tt_want_int_op(trj.w0, >=, command.speed_start);
                    } else {
                        tt_want_int_op(trj.w0, ==, command.speed_start);
                    }

                    // Verify that the target speed is reached, which should
                    // always be the case in an infinite maneuver.
                    if (command.speed_target > command.speed_max) {
                        tt_want_int_op(trj.w1, ==, command.speed_max);
                    } else if (command.speed_target < -command.speed_max) {
                        tt_want_int_op(trj.w1, ==, -command.speed_max);
                    } else {
                        tt_want_int_op(trj.w1, ==, command.speed_target);
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
