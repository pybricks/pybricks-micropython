// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <pbio/math.h>
#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

static void test_sqrt(void *env) {
    tt_want(pbio_math_sqrt(0) == 0);
    tt_want(pbio_math_sqrt(1) == 1);
    tt_want(pbio_math_sqrt(4) == 2);
    tt_want(pbio_math_sqrt(400) == 20);
    tt_want(pbio_math_sqrt(40000) == 200);
    tt_want(pbio_math_sqrt(400000000) == 20000);
}

static void test_mul_i32_fix16(void *env) {
    // fix16_maximum == 32767.99998474121
    // fix16_minimum == -32768.0
    tt_want_int_op(pbio_math_mul_i32_fix16(0, F16(0.0)), ==, 0);
    tt_want_int_op(pbio_math_mul_i32_fix16(1, fix16_maximum), ==, 32768);
    tt_want_int_op(pbio_math_mul_i32_fix16(1, fix16_minimum), ==, -32768);
    tt_want_int_op(pbio_math_mul_i32_fix16(2, fix16_maximum), ==, 65536);
    tt_want_int_op(pbio_math_mul_i32_fix16(2, fix16_minimum), ==, -65536);
    tt_want_int_op(pbio_math_mul_i32_fix16(32768, fix16_maximum), ==, 1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(32768, fix16_minimum), ==, -1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(65535, fix16_maximum), ==, 2147450879);
    tt_want_int_op(pbio_math_mul_i32_fix16(65535, fix16_minimum), ==, -2147450880);
    // we can safely multiply up to 2^16 with arbitrary fix16 value.
    tt_want_int_op(pbio_math_mul_i32_fix16(65536, fix16_maximum), ==, INT32_MAX);
    tt_want_int_op(pbio_math_mul_i32_fix16(65536, fix16_minimum), ==, INT32_MIN);
    tt_want_int_op(pbio_math_mul_i32_fix16(-65536, fix16_maximum), ==, INT32_MIN + 1);
    tt_want_int_op(pbio_math_mul_i32_fix16(-65536, fix16_minimum), ==, INT32_MIN); // overflow!
    // we can safely multiply up to +/-INT32_MAX with fix16 values less than or equal to one.
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MAX, F16(0.5)), ==, 1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MIN, F16(0.5)), ==, -1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MAX, F16(1.0)), ==, INT32_MAX);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MIN, F16(1.0)), ==, INT32_MIN);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MAX, F16(-1.0)), ==, INT32_MIN + 1);
    tt_want_int_op(pbio_math_mul_i32_fix16(-INT32_MAX, F16(-1.0)), ==, INT32_MAX);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MIN, F16(-1.0)), ==, INT32_MIN); // overflow!
}

static void test_div_i32_fix16(void *env) {
    tt_want_int_op(pbio_math_div_i32_fix16(1, F16(0.5)), ==, 2);
    tt_want_int_op(pbio_math_div_i32_fix16(-1, F16(0.5)), ==, -2);
    tt_want_int_op(pbio_math_div_i32_fix16(1, F16(-0.5)), ==, -2);
    tt_want_int_op(pbio_math_div_i32_fix16(-1, F16(-0.5)), ==, 2);
    tt_want_int_op(pbio_math_div_i32_fix16(1e9, F16(0.5)), ==, 2000000000);
    tt_want_int_op(pbio_math_div_i32_fix16(-1e9, F16(0.5)), ==, -2000000000);
    tt_want_int_op(pbio_math_div_i32_fix16(1e9, F16(-0.5)), ==, -2000000000);
    tt_want_int_op(pbio_math_div_i32_fix16(-1e9, F16(-0.5)), ==, 2000000000);
    // we can safely divide up to +/-INT32_MAX by fix16 values greater than or equal to one.
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MAX, fix16_maximum), ==, 65536);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MIN, fix16_maximum), ==, -65536);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MAX, fix16_minimum), ==, -65536);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MIN, fix16_minimum), ==, 65536);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MAX, F16(2.0)), ==, 1073741824);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MIN, F16(2.0)), ==, -1073741824);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MAX, F16(1.0)), ==, INT32_MAX);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MIN, F16(1.0)), ==, INT32_MIN);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MAX, F16(-1.0)), ==, INT32_MIN + 1);
    tt_want_int_op(pbio_math_div_i32_fix16(-INT32_MAX, F16(-1.0)), ==, INT32_MAX);
    tt_want_int_op(pbio_math_div_i32_fix16(INT32_MIN, F16(-1.0)), ==, INT32_MIN); // overflow!
}

struct testcase_t pbio_math_tests[] = {
    PBIO_TEST(test_sqrt),
    PBIO_TEST(test_mul_i32_fix16),
    PBIO_TEST(test_div_i32_fix16),
    END_OF_TESTCASES
};
