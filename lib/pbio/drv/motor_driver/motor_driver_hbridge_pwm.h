// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Driver for motor driver chips that use an H-bridge driven by PWMs.

#ifndef _INTERNAL_PBDRV_MOTOR_DRIVER_HBRIDGE_PWM_H_
#define _INTERNAL_PBDRV_MOTOR_DRIVER_HBRIDGE_PWM_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM

#include <stdint.h>

#include <pbdrv/gpio.h>

typedef struct {
    pbdrv_gpio_t pin1_gpio;
    uint8_t pin1_alt;
    uint8_t pin1_pwm_id;
    uint8_t pin1_pwm_ch;
    pbdrv_gpio_t pin2_gpio;
    uint8_t pin2_alt;
    uint8_t pin2_pwm_id;
    uint8_t pin2_pwm_ch;
} pbdrv_motor_driver_hbridge_pwm_platform_data_t;

extern const pbdrv_motor_driver_hbridge_pwm_platform_data_t
    pbdrv_motor_driver_hbridge_pwm_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV];

#endif // PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM

#endif // _INTERNAL_PBDRV_MOTOR_DRIVER_HBRIDGE_PWM_H_
