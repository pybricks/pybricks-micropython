// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_EV3

#include <stdbool.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>

void pbdrv_battery_init(void) {
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    // TODO. Return nominal value for now so we don't trigger low battery shutdown.
    *value = 7200;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    // TODO
    *value = PBDRV_BATTERY_TYPE_UNKNOWN;
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY_EV3
