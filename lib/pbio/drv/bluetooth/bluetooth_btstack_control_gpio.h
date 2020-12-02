// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// GPIO power controller for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_CONTROL_GPIO_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_CONTROL_GPIO_H_

#include <btstack_control.h>

#include <pbdrv/gpio.h>

const btstack_control_t *pbdrv_bluetooth_btstack_control_gpio_instance(void);


/** BlueKitchen BTStack GPIO power control driver platform-specific data. */
typedef struct {
    /** GPIO connected to enable pin. */
    pbdrv_gpio_t enable_gpio;
} pbdrv_bluetooth_btstack_control_gpio_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_control_gpio_platform_data_t
    pbdrv_bluetooth_btstack_control_gpio_platform_data;

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_CONTROL_GPIO_H_
