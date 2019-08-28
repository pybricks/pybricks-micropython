// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

// Battery driver that uses the mainline Linux kernel driver for LEGO
// MINDSTORMS EV3

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_LINUX_EV3

#include <stdbool.h>
#include <stdio.h>

#include <pbio/error.h>
#include <sys/process.h>

PROCESS(pbdrv_battery_process, "battery");

static FILE *f_voltage;
static FILE *f_current;

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

static void pbdrv_battery_exit(void) {
    if (f_voltage != NULL) {
        fclose(f_voltage);
    }
    if (f_current != NULL) {
        fclose(f_current);
    }
}

PROCESS_THREAD(pbdrv_battery_process, ev, data) {
    PROCESS_EXITHANDLER(pbdrv_battery_exit());

    PROCESS_BEGIN();

    f_voltage = fopen("/sys/class/power_supply/lego-ev3-battery/voltage_now", "r");
    if (!f_voltage) {
        perror("Battery voltage init failed");
    } else {
        setbuf(f_voltage, NULL);
    }

    f_current = fopen("/sys/class/power_supply/lego-ev3-battery/current_now", "r");
    if (!f_current) {
        perror("Battery current init failed");
    } else {
        setbuf(f_voltage, NULL);
    }

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BATTERY_LINUX_EV3
