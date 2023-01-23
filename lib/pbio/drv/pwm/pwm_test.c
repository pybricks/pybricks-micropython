// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TEST

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/pwm.h>
#include <pbio/error.h>
#include <test-pbio.h>

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

#endif // PBDRV_CONFIG_PWM_TEST
