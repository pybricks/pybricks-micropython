// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// GPIO power controller for BlueKitchen BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X

#include <btstack.h>

#include <pbdrv/gpio.h>

#include "bluetooth_btstack_control_gpio.h"


static int btstack_control_gpio_on() {
    const pbdrv_bluetooth_btstack_control_gpio_platform_data_t *pdata =
        &pbdrv_bluetooth_btstack_control_gpio_platform_data;

    pbdrv_gpio_out_high(&pdata->enable_gpio);

    return 0;
}

static int btstack_control_gpio_off() {
    const pbdrv_bluetooth_btstack_control_gpio_platform_data_t *pdata =
        &pbdrv_bluetooth_btstack_control_gpio_platform_data;

    pbdrv_gpio_out_low(&pdata->enable_gpio);

    return 0;
}

static void btstack_control_gpio_init(const void *config) {
    btstack_control_gpio_off();
}

static const btstack_control_t btstack_control_gpio = {
    .init = btstack_control_gpio_init,
    .on = btstack_control_gpio_on,
    .off = btstack_control_gpio_off,
    .sleep = NULL,
    .wake = NULL,
    .register_for_power_notifications = NULL,
};

const btstack_control_t *pbdrv_bluetooth_btstack_control_gpio_instance(void) {
    return &btstack_control_gpio;
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X
