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
    tt_want_int_op(pbio_math_sqrt(0), ==, 0);
    tt_want_int_op(pbio_math_sqrt(1), ==, 1);
    tt_want_int_op(pbio_math_sqrt(4), ==, 2);
    tt_want_int_op(pbio_math_sqrt(400), ==, 20);
    tt_want_int_op(pbio_math_sqrt(40000), ==, 200);
    tt_want_int_op(pbio_math_sqrt(400000000), ==, 20000);

    // Negative square roots do not exist but are expected to return 0.
    tt_want_int_op(pbio_math_sqrt(-36), ==, 0);

    for (int32_t s = 0; s < INT32_MAX - 255; s += 256) {
        tt_want_int_op(pbio_math_sqrt(s), ==, sqrt(s));
    }
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

    // Number of values to test for each input. Higher means more testing,
    // but takes longer.
    const int32_t steps = 256;

    const int64_t input_max = INT32_MAX;
    const int64_t result_max = INT32_MAX;
    const int64_t scale_max = UINT16_MAX;
    const int64_t product_max = (((int64_t)1) << 48) - 1;

    // Let input (a) vary from minimum to maximum.
    for (int64_t a = -input_max; a < input_max; a += input_max / steps) {
        // Let input (b) vary from minimum to maximum.
        for (int64_t b = -input_max; b < input_max; b += input_max / steps) {
            // Let input (c) vary from minimum to maximum.
            for (int64_t c = -scale_max; c < scale_max; c += scale_max / steps) {

                // Skip zero division.
                if (c == 0) {
                    continue;
                }

                // Get the long result.
                int64_t result = a * b / c;

                // Skip pairs too big for testing.
                if (a * b > product_max || a * b < -product_max ||
                    result > result_max || result < -result_max) {
                    continue;
                }

                tt_want_int_op(result, ==, pbio_math_mult_then_div(a, b, c));
            }
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
