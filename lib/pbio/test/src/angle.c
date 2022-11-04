// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <pbio/angle.h>
#include <pbio/int_math.h>

#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

/**
 * Test angle operations.
 */
static void test_rounding(void *env) {
    pbio_angle_t rot_p1_mdeg_p750 = {
        .rotations = 1,
        .millidegrees = 750,
    };
    tt_want_int_op(pbio_angle_to_low_res(&rot_p1_mdeg_p750, 1000), ==, 361);
}

static int64_t get_mdeg(pbio_angle_t *a) {
    return (int64_t)a->rotations * (int64_t)360000 + (int64_t)a->millidegrees;
}

static int32_t get_random() {
    return rand() - RAND_MAX / 2;
}

/**
 * Test binary operations and scaling.
 */
static void test_scale(int32_t scale) {
    for (uint32_t i = 0; i < 1000; i++) {

        // Random angle a.
        pbio_angle_t a = {
            .rotations = get_random(),
            .millidegrees = get_random(),
        };

        // Random angle b.
        pbio_angle_t b = {
            .rotations = get_random(),
            .millidegrees = get_random(),
        };

        // Random smaller angle c.
        pbio_angle_t c = {
            // Such that resulting degrees fit in int32_t.
            .rotations = get_random() % (INT32_MAX / 360 / 4),
            .millidegrees = get_random(),
        };

        // Test negation.
        pbio_angle_t a_neg = a;
        pbio_angle_neg(&a_neg);
        tt_want_int_op(-get_mdeg(&a), ==, get_mdeg(&a_neg));

        // Test addition.
        pbio_angle_t result;
        pbio_angle_sum(&a, &b, &result);
        tt_want_int_op(get_mdeg(&a) + get_mdeg(&b), ==, get_mdeg(&result));

        // Test subtraction.
        pbio_angle_diff(&a, &b, &result);
        tt_want_int_op(get_mdeg(&a) - get_mdeg(&b), ==, get_mdeg(&result));

        // Test average.
        pbio_angle_avg(&a, &b, &result);
        int64_t err = get_mdeg(&a) + get_mdeg(&b) - get_mdeg(&result) * 2;
        tt_want_int_op(err, >=, -1);
        tt_want_int_op(err, <=, 1);

        // Test scaling down.
        int32_t scaled_down = pbio_angle_to_low_res(&c, scale);
        int32_t error = roundf(get_mdeg(&c) / (float)scale) - scaled_down;

        // FIXME: This should be 0!
        tt_want_int_op(pbio_int_math_abs(error), <=, 32);
    }
}

/**
 * Test scaling for typical scaling values.
 */
static void test_scaling(void *env) {
    test_scale(1000); // Single motor: 1000 mdeg = 1 deg
    test_scale(2000); // Medium drive base distance control: 2000 mdeg = 1mm
    test_scale(2046); // Medium drive base heading control: 2046 mdeg = 1deg
}

struct testcase_t pbio_angle_tests[] = {
    PBIO_TEST(test_rounding),
    PBIO_TEST(test_scaling),
    END_OF_TESTCASES
};
