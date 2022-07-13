// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

#include <stdio.h>

#include <math.h>

#include <pbio/math.h>
#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

static void test_clamp(void *env) {
    tt_want_int_op(pbio_math_clamp(200, 100), ==, 100);
    tt_want_int_op(pbio_math_clamp(50, 100), ==, 50);
    tt_want_int_op(pbio_math_clamp(-50, 100), ==, -50);
    tt_want_int_op(pbio_math_clamp(-200, 100), ==, -100);
}

static void test_sqrt(void *env) {
    tt_want(pbio_math_sqrt(0) == 0);
    tt_want(pbio_math_sqrt(1) == 1);
    tt_want(pbio_math_sqrt(4) == 2);
    tt_want(pbio_math_sqrt(400) == 20);
    tt_want(pbio_math_sqrt(40000) == 200);
    tt_want(pbio_math_sqrt(400000000) == 20000);
}

static void test_atan2(void *env) {

    // Test the atan2 function directly.
    for (int32_t x = -1000; x < 1000; x++) {
        for (int32_t y = -1000; y < 1000; y++) {

            // Get real result and our approximation.
            int32_t real = atan2(y, x) / M_PI * 180;
            int32_t ours = pbio_math_atan2(y, x);

            // Get resulting error.
            int32_t error = pbio_math_abs(real - ours);
            if (error >= 180) {
                error -= 360;
            }

            // Assert that error remains small.
            tt_want_int_op(error, <=, 3);
        }
    }

    // Test atan2 to simulate getting the roll angle around a circle.
    // This pushes the boundary less, so we can ask for a closer match.
    for (int32_t angle_in = -180; angle_in < 180; angle_in++) {

        // Repeat tests for different scales.
        for (int32_t scale = 100; scale < 100000; scale += 100) {

            // Acceleration for current angle.
            double rad = (double)angle_in * M_PI / 180;
            double x = cos(rad) * scale;
            double y = sin(rad) * scale;

            // Test atan2 output.
            int32_t angle_out = pbio_math_atan2(y, x);

            // Get resulting error.
            int32_t error = pbio_math_abs(angle_in - angle_out);
            if (error >= 180) {
                error -= 360;
            }

            // Assert that error remains small.
            tt_want_int_op(error, <=, 2);
        }
    }
}

static void test_mult_and_scale(void *env) {

    // We use this function primarily to multiply speed and acceleration by
    // time and divide by 100 or 1000, so test the ranges accordingly. The test
    // increments are arbitrary so the test doesn't take too long.
    for (int32_t w = -40000; w < 40000; w += 40) {
        for (int32_t t = 0; t < 5360000; t += 134) {

            // Get full long product for comparison.
            int64_t product = (int64_t)w * (int64_t)t;

            int32_t div100 = pbio_math_mult_and_scale(w, t, 100);
            int32_t div1000 = pbio_math_mult_and_scale(w, t, 1000);

            tt_want_int_op(div100, ==, product / 100);
            tt_want_int_op(div1000, ==, product / 1000);
        }
    }
}

struct testcase_t pbio_math_tests[] = {
    PBIO_TEST(test_atan2),
    PBIO_TEST(test_clamp),
    PBIO_TEST(test_mult_and_scale),
    PBIO_TEST(test_sqrt),
    END_OF_TESTCASES
};
