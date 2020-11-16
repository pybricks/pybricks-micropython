// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Hooks for unit tests.

#ifndef _PBDRV_PWM_TEST_H_
#define _PBDRV_PWM_TEST_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TEST

#include <pbdrv/pwm.h>

void pbdrv_pwm_test_init(pbdrv_pwm_dev_t *devs);
void pbdrv_pwm_test_deinit(pbdrv_pwm_dev_t *devs);

#else // PBDRV_CONFIG_PWM_TEST

#define pbdrv_pwm_test_init(dev)
#define pbdrv_pwm_test_deinit(dev)

#endif // PBDRV_CONFIG_PWM_TEST

#endif // _PBDRV_PWM_TEST_H_
