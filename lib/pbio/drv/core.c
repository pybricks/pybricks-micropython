// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <contiki.h>

#include "core.h"
#include "battery/battery.h"
#include "counter/counter.h"
#include "led/led_array.h"
#include "led/led.h"
#include "pwm/pwm.h"

uint32_t pbdrv_init_busy_count;
uint32_t pbdrv_deinit_busy_count;

/** Initializes all enabled drivers. */
void pbdrv_init() {
    clock_init();
    process_init();
    process_start(&etimer_process, NULL);
    pbdrv_battery_init();
    pbdrv_counter_init();
    pbdrv_led_init();
    pbdrv_led_array_init();
    pbdrv_pwm_init();
    while (pbdrv_init_busy()) {
        process_run();
    }
}

/**
 * Deinitializes drivers.
 *
 * This is intended to handle things like turning off lights on power down, so
 * not all drivers need to have a deinit. This is needed since power doesn't
 * actually turn off until the button is released and the USB cable is disconnected
 * on Powered Up hubs, so we need to make it look like the power is off already
 * so that users will know when to release the button.
 *
 * Some drivers still need to be useable after this, like reset and battery
 * charger drivers.
 */
void pbdrv_deinit() {
    // REVISIT: It probably makes more sense to have a pbio_deinit() that stops
    // things at a higher level instead of pbdrv_deinit(). But that isn't
    // possible right now, e.g. since pbio_light* isn't aware of the async nature
    // of the underlying drivers.

    // TODO: need to power down I/O ports here
    pbdrv_pwm_deinit();
    while (pbdrv_deinit_busy()) {
        process_run();
    }
}
