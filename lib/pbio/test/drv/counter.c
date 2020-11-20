// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/counter.h>
#include <pbio/error.h>

#include "../drv/counter/counter.h"

typedef struct {
    int32_t count;
    int32_t abs_count;
    int32_t rate;
} test_private_data_t;

static test_private_data_t test_private_data;

// Functions for tests to poke counter state

void pbio_test_counter_set_count(int32_t count) {
    test_private_data.count = count;
}

void pbio_test_counter_set_abs_count(int32_t count) {
    test_private_data.abs_count = count;
}

void pbio_test_counter_set_rate(int32_t rate) {
    test_private_data.rate = rate;
}

// Counter driver implementation

static pbio_error_t test_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    test_private_data_t *priv = dev->priv;
    *count = priv->count;
    return PBIO_SUCCESS;
}

static pbio_error_t test_get_abs_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    test_private_data_t *priv = dev->priv;
    *count = priv->abs_count;
    return PBIO_SUCCESS;
}

static pbio_error_t test_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    test_private_data_t *priv = dev->priv;
    *rate = priv->rate;
    return PBIO_SUCCESS;
}

static const pbdrv_counter_funcs_t test_funcs = {
    .get_count = test_get_count,
    .get_abs_count = test_get_abs_count,
    .get_rate = test_get_rate,
};

void pbdrv_counter_test_init(pbdrv_counter_dev_t *devs) {
    devs[0].funcs = &test_funcs;
    devs[0].priv = &test_private_data;
}

// Tests

void test_counter_get(void *env) {
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
