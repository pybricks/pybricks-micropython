// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/counter.h>
#include <pbio/error.h>
#include <test-pbio.h>

#include "../drv/counter/counter.h"

typedef struct {
    int32_t rotations;
    int32_t millidegrees;
} test_private_data_t;

static test_private_data_t test_private_data;

// Functions for tests to poke counter state

void pbio_test_counter_set_angle(int32_t rotations, int32_t millidegrees) {
    test_private_data.rotations = rotations;
    test_private_data.millidegrees = millidegrees;
}

void pbio_test_counter_set_abs_count(int32_t millidegrees) {
    test_private_data.millidegrees = millidegrees;
}

// Counter driver implementation

static pbio_error_t test_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    test_private_data_t *priv = dev->priv;
    *millidegrees = priv->millidegrees;
    *rotations = priv->rotations;
    return PBIO_SUCCESS;
}

static pbio_error_t test_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {
    test_private_data_t *priv = dev->priv;
    *millidegrees = priv->millidegrees;
    return PBIO_SUCCESS;
}

static const pbdrv_counter_funcs_t test_funcs = {
    .get_angle = test_get_angle,
    .get_abs_angle = test_get_abs_angle,
};

void pbdrv_counter_test_init(pbdrv_counter_dev_t *devs) {
    devs[0].funcs = &test_funcs;
    devs[0].priv = &test_private_data;
}

// Tests

static void test_counter_get(void *env) {
    pbdrv_counter_dev_t *dev;

    // call before initialization
    tt_want(pbdrv_counter_get_dev(0, &dev) == PBIO_ERROR_AGAIN);

    // bad id
    tt_want(pbdrv_counter_get_dev(1, &dev) == PBIO_ERROR_NO_DEV);

    // proper usage
    pbdrv_counter_init();
    tt_want(pbdrv_counter_get_dev(0, &dev) == PBIO_SUCCESS);
    tt_want(dev->priv == &test_private_data);
}

struct testcase_t pbdrv_counter_tests[] = {
    PBIO_TEST(test_counter_get),
    END_OF_TESTCASES
};
