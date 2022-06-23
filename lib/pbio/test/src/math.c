// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

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

struct testcase_t pbio_math_tests[] = {
    PBIO_TEST(test_clamp),
    PBIO_TEST(test_sqrt),
    END_OF_TESTCASES
};
