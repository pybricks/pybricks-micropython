// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/pwm.h>
#include <pbio/error.h>
#include <test-pbio.h>

#include "../drv/pwm/pwm.h"

static void test_pwm_get(void *env) {
    pbdrv_pwm_dev_t *dev;

    // call before initialization
    tt_want(pbdrv_pwm_get_dev(0, &dev) == PBIO_ERROR_AGAIN);

    // bad id
    tt_want(pbdrv_pwm_get_dev(1, &dev) == PBIO_ERROR_NO_DEV);

    // proper usage
    pbdrv_pwm_init();
    tt_want(pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS);
}

static void test_pwm_set_duty(void *env) {
    pbdrv_pwm_dev_t *dev;

    pbdrv_pwm_init();
    tt_want(pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS);
    tt_want(pbdrv_pwm_set_duty(dev, 1, 100) == PBIO_SUCCESS);
}

struct testcase_t pbdrv_pwm_tests[] = {
    PBIO_TEST(test_pwm_get),
    PBIO_TEST(test_pwm_set_duty),
    END_OF_TESTCASES
};
