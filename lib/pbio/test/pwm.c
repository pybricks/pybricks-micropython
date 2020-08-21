// SPDX-License-Identifier: MIT
// Copyright 2020 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/pwm.h>
#include <pbio/error.h>

#include "../drv/pwm/pwm.h"

typedef struct {
    uint32_t duty_channel;
    uint32_t duty_value;
} test_private_data_t;

static test_private_data_t test_private_data;

static pbio_error_t test_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    test_private_data_t *priv = dev->priv;
    priv->duty_channel = ch;
    priv->duty_value = value;
    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t test_funcs = {
    .set_duty = test_set_duty,
};

void pbdrv_pwm_test_init(pbdrv_pwm_dev_t *devs) {
    devs[0].funcs = &test_funcs;
    devs[0].priv = &test_private_data;
}

void test_pwm_get(void *env) {
    pbdrv_pwm_dev_t *dev;

    // call before initialization
    tt_want(pbdrv_pwm_get_dev(0, &dev) == PBIO_ERROR_AGAIN);

    // bad id
    tt_want(pbdrv_pwm_get_dev(1, &dev) == PBIO_ERROR_NO_DEV);

    // proper usage
    pbdrv_pwm_init();
    tt_want(pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS);
    tt_want(dev->priv == &test_private_data);

}

void test_pwm_set_duty(void *env) {
    pbdrv_pwm_dev_t *dev;

    pbdrv_pwm_init();
    tt_want(pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS);
    tt_want(pbdrv_pwm_set_duty(dev, 1, 100) == PBIO_SUCCESS);
    tt_want_int_op(test_private_data.duty_channel, ==, 1);
    tt_want_int_op(test_private_data.duty_value, ==, 100);
}
