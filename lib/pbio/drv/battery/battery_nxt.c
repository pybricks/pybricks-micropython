// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Battery driver that uses the LeJOS drivers for NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_NXT

#include <stdbool.h>
#include <stdio.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>

#include "../rproc/rproc_nxt.h"

void pbdrv_battery_init(void) {
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    pbdrv_rproc_nxt_get_battery_info(value);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    uint16_t voltage;
    *value = pbdrv_rproc_nxt_get_battery_info(&voltage) ? PBDRV_BATTERY_TYPE_LIION : PBDRV_BATTERY_TYPE_ALKALINE;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY_NXT
