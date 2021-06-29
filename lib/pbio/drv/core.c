// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <contiki.h>

#include "core.h"
#include "battery/battery.h"
#include "bluetooth/bluetooth.h"
#include "counter/counter.h"
#include "ioport/ioport.h"
#include "led/led_array.h"
#include "led/led.h"
#include "pwm/pwm.h"
#include "reset/reset.h"
#include "sound/sound.h"
#include "watchdog/watchdog.h"

uint32_t pbdrv_init_busy_count;

/** Initializes all enabled drivers. */
void pbdrv_init(void) {
    clock_init();
    process_init();
    process_start(&etimer_process, NULL);
    pbdrv_battery_init();
    pbdrv_bluetooth_init();
    pbdrv_counter_init();
    pbdrv_ioport_init();
    pbdrv_led_array_init();
    pbdrv_led_init();
    pbdrv_pwm_init();
    pbdrv_reset_init();
    pbdrv_sound_init();
    pbdrv_watchdog_init();
    while (pbdrv_init_busy()) {
        process_run();
    }
}
