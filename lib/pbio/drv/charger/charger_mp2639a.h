// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// driver for MPS MP2639A USB battery charger chip

#ifndef _INTERNAL_CHARGER_MP2639A_H_
#define _INTERNAL_CHARGER_MP2639A_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#if PBDRV_CONFIG_CHARGER_MP2639A_CHG_RESISTOR_LADDER
#include <pbdrv/resistor_ladder.h>
#endif

typedef struct {
    // SPIKE Prime quirk: The MODE pin is connected to the LED controller
    // instead of being directly connected to a GPIO on the MCU.
    #if PBDRV_CONFIG_CHARGER_MP2639A_MODE_PWM
    /** ID of PWM device connected to the MODE pin. */
    uint8_t mode_pwm_id;
    /** Channel of PWM connected to the MODE pin. */
    uint8_t mode_pwm_ch;
    #else
    /** GPIO connected to the MODE pin. */
    pbdrv_gpio_t mode_gpio;
    #endif
    // SPIKE Prime quirk: the CHG pin is connected to an analog input shared
    // with the buttons.
    #if PBDRV_CONFIG_CHARGER_MP2639A_CHG_RESISTOR_LADDER
    /** The ID of the resistor ladder the CHG pin is connected to. */
    uint8_t chg_resistor_ladder_id;
    /** The resistor ladder channel the CHG pin is connected to. */
    pbdrv_resistor_ladder_ch_flags_t chg_resistor_ladder_ch;
    #else
    /** GPIO connected to the CHG pin. */
    pbdrv_gpio_t chg_gpio;
    #endif
    /** ADC channel connected to the IB pin. */
    uint8_t ib_adc_ch;
    // TODO: add optional ISET output
} pbdrv_charger_mp2639a_platform_data_t;

extern const pbdrv_charger_mp2639a_platform_data_t pbdrv_charger_mp2639a_platform_data;

#endif // _INTERNAL_CHARGER_MP2639A_H_
