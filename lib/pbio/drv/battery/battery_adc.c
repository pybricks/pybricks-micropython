// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner

// Battery driver that uses an ADC to read battery voltage and current

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_ADC

#include <stdbool.h>

#include <pbdrv/adc.h>

#include <pbio/error.h>
#include <sys/process.h>

PROCESS(pbdrv_battery_process, "battery");

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    uint16_t raw;
    pbio_error_t err;

    err = pbdrv_adc_get_ch(PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH, &raw);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // REVISIT: do we want to take into account shunt resistor voltage drop
    // like on EV3? Probably only makes a difference of ~10mV at the most.
    *value = raw * PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX /
             PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX;

    return PBIO_SUCCESS;
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

    *value = raw * PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX /
             PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX;

    return PBIO_SUCCESS;
}

PROCESS_THREAD(pbdrv_battery_process, ev, data) {
    PROCESS_BEGIN();

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BATTERY_ADC
