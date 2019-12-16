// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

// Battery driver that uses the LeJOS drivers for NXT.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BATTERY_NXT

#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>
#include <nxt/nxt_avr.h>

#include <pbio/error.h>


PROCESS(pbdrv_battery_process, "battery");

pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    *value = battery_voltage() & 0x7FFF;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static void pbdrv_battery_exit(void) {
}

PROCESS_THREAD(pbdrv_battery_process, ev, data) {
    PROCESS_EXITHANDLER(pbdrv_battery_exit());

    PROCESS_BEGIN();

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BATTERY_NXT
