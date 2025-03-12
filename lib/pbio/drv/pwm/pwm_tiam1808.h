// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Hooks for unit tiam1808s.

#ifndef _INTERNAL_PBDRV_PWM_TIAM1808_H_
#define _INTERNAL_PBDRV_PWM_TIAM1808_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TIAM1808

#include <pbdrv/pwm.h>
#include <pbdrv/gpio.h>

/** Platform-specific device information. */
typedef struct {
    uint8_t id;
    pbdrv_gpio_t gpio_red;
    pbdrv_gpio_t gpio_green;
} pbdrv_pwm_tiam1808_platform_data_t;

// Defined in platform.c
extern const pbdrv_pwm_tiam1808_platform_data_t
    pbdrv_pwm_tiam1808_platform_data[PBDRV_CONFIG_PWM_TIAM1808_NUM_DEV];

void pbdrv_pwm_tiam1808_init(pbdrv_pwm_dev_t *devs);

#else // PBDRV_CONFIG_PWM_TIAM1808

#define pbdrv_pwm_tiam1808_init(dev)

#endif // PBDRV_CONFIG_PWM_TIAM1808

#endif // _INTERNAL_PBDRV_PWM_TIAM1808_H_
