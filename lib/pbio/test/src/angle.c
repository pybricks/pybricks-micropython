// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdio.h>
#include <stdlib.h>

#include <pbio/angle.h>

#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

/**
 * Test angle operations.
 */
static void test_angle(void *env) {
    pbio_angle_t rot_p1_mdeg_p750 = {
        .rotations = 1,
        .millidegrees = 750,
    };
    tt_want_int_op(pbio_angle_to_low_res(&rot_p1_mdeg_p750, 1000), ==, 361);
}

struct testcase_t pbio_angle_tests[] = {
    PBIO_TEST(test_angle),
    END_OF_TESTCASES
};
