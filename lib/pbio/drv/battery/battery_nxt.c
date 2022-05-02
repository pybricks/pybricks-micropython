// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Battery driver that uses the LeJOS drivers for NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_NXT

#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>
#include <base/drivers/avr.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>

void pbdrv_battery_init(void) {
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    // REVISIT: Voltage appears off by a factor 2
    *value = nx_avr_get_battery_voltage() * 2;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    *value = nx_avr_battery_is_aa() ? PBDRV_BATTERY_TYPE_ALKALINE : PBDRV_BATTERY_TYPE_LIION;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY_NXT
