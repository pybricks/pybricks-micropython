// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Software battery implementation for simulating battery in tests

#include <pbdrv/battery.h>
#include <pbio/error.h>

void pbdrv_battery_init(void) {
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    *value = 7200;
    return PBIO_SUCCESS;
}
