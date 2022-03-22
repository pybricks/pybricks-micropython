// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdio.h>

#include <pbio/trajectory.h>
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

struct testcase_t pbio_trajectory_tests[] = {
    PBIO_TEST(test_simple_trajectory),
    END_OF_TESTCASES
};
