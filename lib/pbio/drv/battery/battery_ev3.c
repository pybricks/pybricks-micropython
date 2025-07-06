// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_EV3

#include <stdbool.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>
#include <pbdrv/gpio.h>
#include "../gpio/gpio_ev3.h"

#include <tiam1808/hw/hw_syscfg0_AM1808.h>

static const pbdrv_gpio_t battery_type_gpio = PBDRV_GPIO_EV3_PIN(19, 7, 4, 8, 8);

void pbdrv_battery_init(void) {
    pbdrv_gpio_alt(&battery_type_gpio, SYSCFG_PINMUX19_PINMUX19_7_4_GPIO8_8);
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    // TODO. Calculate battery voltage based on ADC channel 4.
    // For now, return nominal value for now so we don't trigger low battery shutdown.
    *value = 7200;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    // TODO. Calculate battery current based on ADC channel 3.
    *value = 0;
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    if (pbdrv_gpio_input(&battery_type_gpio)) {
        *value = PBDRV_BATTERY_TYPE_ALKALINE;
    } else {
        *value = PBDRV_BATTERY_TYPE_LIION;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    // EV3 does not have a way to read the battery temperature.
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY_EV3
