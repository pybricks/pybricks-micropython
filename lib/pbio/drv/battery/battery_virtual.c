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
    return pbdrv_virtual_get_u16("battery", -1, "voltage", value);
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    return pbdrv_virtual_get_u16("battery", -1, "current", value);
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    return pbdrv_virtual_get_u32("battery", -1, "temperature", value);
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    uint8_t int_value;
    pbio_error_t err = pbdrv_virtual_get_u8("battery", -1, "type", &int_value);
    *value = int_value;
    return err;
}

#endif // PBDRV_CONFIG_BATTERY_VIRTUAL
