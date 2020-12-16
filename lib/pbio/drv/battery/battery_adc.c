// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Battery driver that uses an ADC to read battery voltage and current

// Configuration parameters:
//
// PBDRV_CONFIG_BATTERY_ADC:
//      enable/disable driver.
// PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH:
//      ADC channel that measures battery voltage.
// PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX:
//      The max size of the raw value, e.g. 4096 for 12-bit ADC. Prefer power
//      of 2 for smaller code size, e.g. use 4096 instead of 4095.
// PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX:
//      The voltage in mV that corresponds that would result in the raw measured
//      value PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX being read on the ADC.
// PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION:
//      Current correction factor applied to measured battery voltage. Units are
//      1/16 Ω, e.g. 12 => 12/16 Ω = 0.75 Ω.
// PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH:
//      ADC channel that measures battery current.
// PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET:
//      An offset to apply to the raw value before scaling it.
// PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX:
//      The max size of the raw value, e.g. 4096 for 12-bit ADC. Prefer power
//      of 2 for smaller code size, e.g. use 4096 instead of 4095.
// PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX:
//      The current in mA that corresponds that would result in the raw measured
//      value PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX being read on the ADC.
// PBDRV_CONFIG_BATTERY_ADC_TYPE:
//      1 = PBDRV_BATTERY_TYPE_ALKALINE only
//      2 = PBDRV_BATTERY_TYPE_LIION only
//      3 = type indicated by GPIO button

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_ADC

#include <stdbool.h>

#include <contiki.h>

#include <pbdrv/adc.h>
#include <pbdrv/battery.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>

#include "battery_adc.h"

#if PBDRV_CONFIG_BATTERY_ADC_TYPE == 3
static pbdrv_battery_type_t pbdrv_battery_type;
#endif

void pbdrv_battery_init() {
    #if PBDRV_CONFIG_BATTERY_ADC_TYPE == 3
    const pbdrv_battery_adc_platform_data_t *pdata = &pbdrv_battery_adc_platform_data;
    pbdrv_gpio_set_pull(&pdata->gpio, pdata->pull);
    pbdrv_battery_type = pbdrv_gpio_input(&pdata->gpio) ?
        PBDRV_BATTERY_TYPE_ALKALINE : PBDRV_BATTERY_TYPE_LIION;
    #endif
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    uint16_t raw;
    pbio_error_t err;

    // this is measuring the voltage across a 0.05 ohm shunt resistor probably
    // via an op amp with unknown gain.
    err = pbdrv_adc_get_ch(PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH, &raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // REVISIT: Hubs, especial Prime and Inventor hubs seem to have an offset
    // that varies greatly by hub (20 or 40 mA). We could introduce a calibration
    // step to try to determine this offset at boot (before lights, ports, etc.
    // are powered on) if we find that we need more accurate current measurement.

    // NOTE: On Move hub, City hub and Technic hub, current measurement is
    // non-linear at low currents (< ~100 mA) so the reported battery current
    // is not accurate at low currents.

    *value = (raw + PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET) *
        PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX /
        PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    uint16_t raw;
    pbio_error_t err;

    err = pbdrv_adc_get_ch(PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH, &raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    uint16_t current;
    err = pbdrv_battery_get_current_now(&current);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // REVISIT: On Technic hub, only the current to ports A/C affect the voltage
    // measurement since the voltage measurement is after the resettable fuse
    // for those ports. So currently, the battery voltage will be reported as
    // up to several tenths of a volt higher than it actually is if there is
    // high current on ports B/D.
    // NOTE: On Move hub, City hub and Technic hub, current measurement is
    // non-linear at low currents (< ~100 mA) so the reported battery voltage
    // will be about 0.1V lower than it actually is when the current is low.
    // NOTE: On Prime and Inventor hubs, the voltage is measured on the same
    // resettable fuse as the lights. So when all of the lights are on at full
    // brightness, the battery voltage will be reported as a few hundredths of
    // a volt lower than it actually is, which is neglegable.
    *value = raw * PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX /
        PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX + current *
        PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION / 16;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    #if PBDRV_CONFIG_BATTERY_ADC_TYPE == 1
    *value = PBDRV_BATTERY_TYPE_ALKALINE;
    #elif PBDRV_CONFIG_BATTERY_ADC_TYPE == 2
    *value = PBDRV_BATTERY_TYPE_LIION;
    #elif PBDRV_CONFIG_BATTERY_ADC_TYPE == 3
    *value = pbdrv_battery_type;
    #else
    #error "Bad PBDRV_CONFIG_BATTERY_ADC_TYPE value"
    #endif
    return PBIO_SUCCESS;
}

#if PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE

#include <math.h>

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    uint16_t raw;
    pbio_error_t err = pbdrv_adc_get_ch(PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE_CH, &raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // This is hard-coded for a 103AT thermistor as found in SPIKE Prime.
    // Refer to MP2639A data sheet for more info on how circuit is configured.

    // Measured resistance of the thermocouple (in Ohms):
    // R_NTC = 1 / (((VNTC / NTC) - 1) / R_T1) - (1 / R_T2))
    //
    // VNTC: the reference voltage
    // NTC: the measured voltage
    // R_T1: 2.4k - resistor
    // R_T2: 7.5k - resistor
    //
    // Voltages are relative so 4095 / raw == VNTC / NTC (assuming 12-bit ADC)

    float r_ntc = 1.0f / (((4095.0f / raw - 1.0f) / 2400.0f) - 1.0f / 7500.0f);

    // Measured temperature of the thermocouple (in Kelvin):
    // T1 = B / (ln(R1 / R2) + B / T2)
    //
    // B: 3435 - temperature coefficient from 103AT data sheet
    // R1: R_NTC - measured thermocouple resistance
    // R2: 10k - 25°C resistance from 103AT data sheet
    // T2: 298.15 - 25°C converted to Kelvin

    float t = 3435.0f / (logf(r_ntc / 10000.0f) + 3435.0f / 298.15f);

    // convert Kelvin to millidegrees C
    *value = (t - 273.15f) * 1000.0f;

    return PBIO_SUCCESS;
}

#else // PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE

#endif // PBDRV_CONFIG_BATTERY_ADC
