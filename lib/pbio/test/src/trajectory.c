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
        .type = PBIO_TRAJECTORY_TYPE_ANGLE,
        .t0 = 0,
        .th0 = 0,
        .th0_ext = 0,
        .th3 = 10000,
        .w0 = 0,
        .wt = 1000,
        .wmax = 1000,
        .a0_abs = 2000,
        .a2_abs = 2000,
        .continue_running = false,
    };

    pbio_trajectory_t trj;
    pbio_error_t err = pbio_trajectory_calculate_new(&trj, &command);
    tt_want_int_op(err, ==, PBIO_SUCCESS);

    tt_want_int_op(trj.t0, ==, command.t0);
    tt_want_int_op(trj.t1 - command.t0, ==, 500 * US_PER_MS);
    tt_want_int_op(trj.t2 - command.t0, ==, 10000 * US_PER_MS);
    tt_want_int_op(trj.t3 - command.t0, ==, 10500 * US_PER_MS);
    tt_want_int_op(trj.th0, ==, command.th0);
    tt_want_int_op(trj.th1, ==, 250);
    tt_want_int_op(trj.th2, ==, 9750);
    tt_want_int_op(trj.th3, ==, command.th3);
    tt_want_int_op(trj.th0_ext, ==, 0);
    tt_want_int_op(trj.th1_ext, ==, 0);
    tt_want_int_op(trj.th2_ext, ==, 0);
    tt_want_int_op(trj.th3_ext, ==, 0);
    tt_want_int_op(trj.w0, ==, 0);
    tt_want_int_op(trj.w1, ==, command.wt);
    tt_want_int_op(trj.w3, ==, 0);
    tt_want_int_op(trj.a0, ==, command.a0_abs);
    tt_want_int_op(trj.a2, ==, -command.a2_abs);
}

static void test_infinite_trajectory(void *env) {

    int32_t accelerations[] = { // FIXME: Large accelerations are not working.
        10, 50, 100, 500, 1000, 2000, // 5000, 10000, 20000
    };

    int32_t speeds[] = {
        -10, -10000, -1000, -500, -10, 0, 10, 500, 1000, 10000
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
                        .type = PBIO_TRAJECTORY_TYPE_TIME,
                        .t0 = times[t],
                        .duration = DURATION_MAX_MS * US_PER_MS,
                        .th0 = 0,
                        .th0_ext = 0,
                        .w0 = speeds[w0],
                        .wt = speeds[wt],
                        .wmax = 1000,
                        .a0_abs = accelerations[a],
                        .a2_abs = accelerations[a],
                        .continue_running = true,
                    };

                    // Calculate the trajectory.
                    pbio_trajectory_t trj;
                    pbio_error_t err = pbio_trajectory_calculate_new(&trj, &command);
                    tt_want_int_op(err, ==, PBIO_SUCCESS);
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
