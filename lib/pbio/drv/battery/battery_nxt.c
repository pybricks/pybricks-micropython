// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Battery driver that uses the LeJOS drivers for NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_NXT

#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>
#include <nxt/nxt_avr.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>

void pbdrv_battery_init() {
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    // Battery voltage includes bit 0x8000 to indicate rechargeable battery
    *value = battery_voltage() & 0x7FFF;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    // Battery voltage includes bit 0x8000 to indicate rechargeable battery
    *value = (battery_voltage() & 0x8000) ? PBDRV_BATTERY_TYPE_LIION
                                          : PBDRV_BATTERY_TYPE_ALKALINE;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BATTERY_NXT
