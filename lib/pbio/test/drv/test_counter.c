// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/counter.h>
#include <pbio/error.h>
#include <test-pbio.h>

#include "../drv/counter/counter.h"

static void test_counter_get(void *env) {
    pbdrv_counter_dev_t *dev;

    // call before initialization
    tt_want(pbdrv_counter_get_dev(0, &dev) == PBIO_ERROR_AGAIN);

    // bad id
    tt_want(pbdrv_counter_get_dev(7, &dev) == PBIO_ERROR_NO_DEV);

    // proper usage
    pbdrv_counter_init();
    tt_want(pbdrv_counter_get_dev(0, &dev) == PBIO_SUCCESS);
}

struct testcase_t pbdrv_counter_tests[] = {
    PBIO_TEST(test_counter_get),
    END_OF_TESTCASES
};
