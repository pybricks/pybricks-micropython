// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Virtual battery that is implemented in Python.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_VIRTUAL

#include <stdint.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>

#include "../virtual.h"

void pbdrv_battery_init(void) {
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    *value = pbdrv_virtual_get_unsigned_long("battery_voltage");

    return *value == (uint16_t)-1 ? PBIO_ERROR_FAILED : PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = pbdrv_virtual_get_unsigned_long("battery_current");

    return *value == (uint16_t)-1 ? PBIO_ERROR_FAILED : PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = pbdrv_virtual_get_unsigned_long("battery_temperature");

    return *value == (uint32_t)-1 ? PBIO_ERROR_FAILED : PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    *value = pbdrv_virtual_get_unsigned_long("battery_type");

    return *value == (pbdrv_battery_type_t)-1 ? PBIO_ERROR_FAILED : PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BATTERY_VIRTUAL
