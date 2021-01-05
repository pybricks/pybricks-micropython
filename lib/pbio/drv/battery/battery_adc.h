// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BATTERY_ADC_H_
#define _INTERNAL_PBDRV_BATTERY_ADC_H_

#include <pbdrv/gpio.h>

// NOTE: Currently platform data is only needed for this driver when
// PBDRV_CONFIG_BATTERY_ADC_TYPE == 3

/**
 * Platform data for ADC battery device.
 */
typedef struct {
    /** Battery type detection GPIO. */
    pbdrv_gpio_t gpio;
    /** Battery type detection GPIO pull setting. */
    pbdrv_gpio_pull_t pull;
} pbdrv_battery_adc_platform_data_t;

// defined in platform.c
extern const pbdrv_battery_adc_platform_data_t pbdrv_battery_adc_platform_data;

#endif // _INTERNAL_PBDRV_BATTERY_ADC_H_
