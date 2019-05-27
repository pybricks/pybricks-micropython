// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY

#include <stdio.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <sys/process.h>

PROCESS(pbdrv_battery_process, "battery");

static FILE *f_voltage;
static FILE *f_current;

pbio_error_t pbdrv_battery_get_voltage_now(pbio_port_t port, uint16_t *value) {
    int32_t microvolt;
    if (0 == fseek(f_voltage, 0, SEEK_SET) &&
        0 <= fscanf(f_voltage, "%d", &microvolt) &&
        0 == fflush(f_voltage)) {
        *value = (microvolt / 1000);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_battery_get_current_now(pbio_port_t port, uint16_t *value) {
    int32_t microamp;
    if (0 == fseek(f_current, 0, SEEK_SET) &&
        0 <= fscanf(f_current, "%d", &microamp) &&
        0 == fflush(f_current)) {
        *value = (microamp / 1000);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
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
    f_current = fopen("/sys/class/power_supply/lego-ev3-battery/current_now", "r");

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BATTERY
