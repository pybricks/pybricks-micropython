// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Battery driver that uses the mainline Linux kernel driver for LEGO
// MINDSTORMS EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_LINUX_EV3

#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>

#include <pbio/error.h>

static FILE *f_voltage;
static FILE *f_current;

void pbdrv_battery_init() {
    f_voltage = fopen("/sys/class/power_supply/lego-ev3-battery/voltage_now", "r");
    if (f_voltage) {
        setbuf(f_voltage, NULL);
    }

    f_current = fopen("/sys/class/power_supply/lego-ev3-battery/current_now", "r");
    if (f_current) {
        setbuf(f_current, NULL);
    }
}

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    int32_t microvolt;

    if (f_voltage == NULL) {
        return PBIO_ERROR_NO_DEV;
    }

    if (fseek(f_voltage, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(f_voltage, "%d", &microvolt) == EOF) {
        return PBIO_ERROR_IO;
    }

    *value = microvolt / 1000;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    int32_t microamp;

    if (f_current == NULL) {
        return PBIO_ERROR_NO_DEV;
    }

    if (fseek(f_current, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    if (fscanf(f_current, "%d", &microamp) == EOF) {
        return PBIO_ERROR_IO;
    }

    *value = microamp / 1000;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BATTERY_LINUX_EV3
