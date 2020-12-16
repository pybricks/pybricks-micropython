// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Battery driver that uses the mainline Linux kernel driver for LEGO
// MINDSTORMS EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_LINUX_EV3

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/battery.h>
#include <pbio/error.h>

static FILE *f_voltage;
static FILE *f_current;
static FILE *f_technology;

void pbdrv_battery_init(void) {
    f_voltage = fopen("/sys/class/power_supply/lego-ev3-battery/voltage_now", "r");
    if (f_voltage) {
        setbuf(f_voltage, NULL);
    }

    f_current = fopen("/sys/class/power_supply/lego-ev3-battery/current_now", "r");
    if (f_current) {
        setbuf(f_current, NULL);
    }

    f_technology = fopen("/sys/class/power_supply/lego-ev3-battery/technology", "r");
    if (f_technology) {
        setbuf(f_technology, NULL);
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

pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    if (f_technology == NULL) {
        return PBIO_ERROR_NO_DEV;
    }

    if (fseek(f_technology, 0, SEEK_SET) == -1) {
        return PBIO_ERROR_IO;
    }

    char *technology;
    if (fscanf(f_technology, "%ms\n", &technology) == EOF) {
        return PBIO_ERROR_IO;
    }

    if (technology && strcmp(technology, "Unknown") == 0) {
        *value = PBDRV_BATTERY_TYPE_ALKALINE;
    } else if (technology && strcmp(technology, "Li-ion") == 0) {
        *value = PBDRV_BATTERY_TYPE_LIION;
    } else {
        *value = PBDRV_BATTERY_TYPE_UNKNOWN;
    }

    if (technology) {
        free(technology);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY_LINUX_EV3
