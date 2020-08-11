// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Driver for 2 individual RGB LEDs that function as a single LED.

#ifndef _PBDRV_LED_DUAL_H_
#define _PBDRV_LED_DUAL_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_DUAL

#include <stdint.h>

#include <pbdrv/led.h>

/** Platform-specific data for dual LED devices. */
typedef struct {
    /** LED device ID. */
    uint8_t id;
    /** ID of first component LED. */
    uint8_t id1;
    /** ID of second component LED. */
    uint8_t id2;
} pbdrv_led_dual_platform_data_t;

// defined in platform/*/platform.c
extern const pbdrv_led_dual_platform_data_t pbdrv_led_dual_platform_data[PBDRV_CONFIG_LED_DUAL_NUM_DEV];

void pbdrv_led_dual_init(pbdrv_led_dev_t *devs);

#else // PBDRV_CONFIG_LED_DUAL

#define pbdrv_led_dual_init(devs)

#endif // PBDRV_CONFIG_LED_DUAL

#endif // _PBDRV_LED_DUAL_H_
