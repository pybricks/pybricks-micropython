// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_EV3

#include <stdbool.h>

#include <pbdrv/adc.h>
#include <pbdrv/battery.h>
#include <pbio/error.h>
#include <pbdrv/gpio.h>
#include "../gpio/gpio_ev3.h"

#include <tiam1808/hw/hw_syscfg0_AM1808.h>

static const pbdrv_gpio_t battery_type_gpio = PBDRV_GPIO_EV3_PIN(19, 7, 4, 8, 8);
static const pbdrv_gpio_t battery_voltage_measure_en_gpio = PBDRV_GPIO_EV3_PIN(1, 7, 4, 0, 6);

#define PBDRV_EV3_BATTERY_VOLTAGE_ADC_CH    4
#define PBDRV_EV3_BATTERY_CURRENT_ADC_CH    3

void pbdrv_battery_init(void) {
    pbdrv_gpio_alt(&battery_type_gpio, SYSCFG_PINMUX19_PINMUX19_7_4_GPIO8_8);
    pbdrv_gpio_out_high(&battery_voltage_measure_en_gpio);
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    uint16_t value_raw;
    pbio_error_t err = pbdrv_adc_get_ch(PBDRV_EV3_BATTERY_VOLTAGE_ADC_CH, &value_raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Battery voltage is read through a voltage divider consisting of two
    // 100 K resistors, which halves the voltage. The ADC returns 10 LSBs,
    // where full scale is 5 V.
    *value = ((uint32_t)(value_raw) * 2 * 5000) / 1023;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    uint16_t value_raw;
    pbio_error_t err = pbdrv_adc_get_ch(PBDRV_EV3_BATTERY_CURRENT_ADC_CH, &value_raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Battery current is read across a 0.05 ohm equivalent shunt resistor
    // which is then connected to an op-amp configured with a gain of 16
    // (non-inverting). This yields 1 A = 0.8 V.
    *value = ((uint32_t)(value_raw) * 5000 * 10) / (1023 * 8);
    return PBIO_SUCCESS;
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
