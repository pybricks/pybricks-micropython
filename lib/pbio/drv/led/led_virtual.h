// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Driver for PWM-driven RGB LED Devices.

#ifndef _INTERNAL_PBDRV_LED_VIRTUAL_H_
#define _INTERNAL_PBDRV_LED_VIRTUAL_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_VIRTUAL

#include <pbdrv/led.h>

void pbdrv_led_virtual_init(pbdrv_led_dev_t *devs);

#else // PBDRV_CONFIG_LED_VIRTUAL

#define pbdrv_led_virtual_init(devs)

#endif // PBDRV_CONFIG_LED_VIRTUAL

#endif // _INTERNAL_PBDRV_LED_VIRTUAL_H_
