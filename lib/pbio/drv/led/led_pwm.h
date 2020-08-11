// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Driver for PWM-driven RGB LED Devices.

#ifndef _PBDRV_LED_PWM_H_
#define _PBDRV_LED_PWM_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_PWM

#include <stdint.h>

#include <pbdrv/led.h>

/** Platform-specific data for PWM LED devices. */
typedef struct {
    /** LED device id. */
    uint8_t id;
    /** Red LED PWM device id. */
    uint8_t r_id;
    /** Red LED PWM channel. */
    uint8_t r_ch;
    /** Green LED PWM device id. */
    uint8_t g_id;
    /** Green LED PWM channel. */
    uint8_t g_ch;
    /** Blue LED PWM device id. */
    uint8_t b_id;
    /** Blue LED PWM channel. */
    uint8_t b_ch;
    /** Scaling factor used to adjust max brightness to match PWM period. */
    uint8_t scale_factor;
} pbdrv_led_pwm_platform_data_t;

// defined in platform/*/platform.c
extern const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV];

void pbdrv_led_pwm_init(pbdrv_led_dev_t *devs);

#else // PBDRV_CONFIG_LED_PWM

#define pbdrv_led_pwm_init(devs)

#endif // PBDRV_CONFIG_LED_PWM

#endif // _PBDRV_LED_PWM_H_
