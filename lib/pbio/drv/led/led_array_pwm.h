// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Driver for PWM-driven LED arrays.

#ifndef _INTERNAL_PBDRV_LED_ARRAY_PWM_H_
#define _INTERNAL_PBDRV_LED_ARRAY_PWM_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_ARRAY_PWM

#include <stdint.h>

#include <pbdrv/led.h>

/** Platform-specific data for PWM LED arrays. */
typedef struct {
    /** Array of PWM device channels. */
    const uint8_t *pwm_chs;
    /** Number of elements in @p pwm_chs. */
    uint8_t num_pwm_chs;
    /** PWM device ID. */
    uint8_t pwm_id;
    /** This LED array's device ID. */
    uint8_t id;
} pbdrv_led_array_pwm_platform_data_t;

// defined in platform/*/platform.c
extern const pbdrv_led_array_pwm_platform_data_t pbdrv_led_array_pwm_platform_data[PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV];

void pbdrv_led_array_pwm_init(pbdrv_led_array_dev_t *devs);

#else // PBDRV_CONFIG_LED_ARRAY_PWM

#define pbdrv_led_array_pwm_init(devs)

#endif // PBDRV_CONFIG_LED_ARRAY_PWM

#endif // _INTERNAL_PBDRV_LED_ARRAY_PWM_H_
