// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Hooks for unit picos.

#ifndef _INTERNAL_PBDRV_PWM_PICO_H_
#define _INTERNAL_PBDRV_PWM_PICO_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_PICO

#include <stdint.h>

#include <pbdrv/pwm.h>

/** Platform-specific device information. */

typedef struct {
    uint8_t gpio;
} pbdrv_pwm_pico_channel_t;

typedef struct {
    uint8_t id;
    pbdrv_pwm_pico_channel_t channels[PBDRV_CONFIG_PWM_PICO_NUM_CHANNELS];
} pbdrv_pwm_pico_platform_data_t;

// Defined in platform.c
extern const pbdrv_pwm_pico_platform_data_t pbdrv_pwm_pico_platform_data;

void pbdrv_pwm_pico_init(pbdrv_pwm_dev_t *devs);

#else // PBDRV_CONFIG_PWM_PICO

#define pbdrv_pwm_pico_init(dev)

#endif // PBDRV_CONFIG_PWM_PICO

#endif // _INTERNAL_PBDRV_PWM_PICO_H_
