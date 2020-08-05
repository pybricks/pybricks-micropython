// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Driver for ev3dev-stretch LED devices.

#ifndef _PBDRV_LED_EV3DEV_H_
#define _PBDRV_LED_EV3DEV_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_EV3DEV

#include <pbdrv/led.h>

void pbdrv_led_ev3dev_init(pbdrv_led_dev_t *devs);

#else // PBDRV_CONFIG_LED_EV3DEV

#define pbdrv_led_ev3dev_init(devs)

#endif // PBDRV_CONFIG_LED_EV3DEV

#endif // _PBDRV_LED_EV3DEV_H_
