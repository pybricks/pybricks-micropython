// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <contiki.h>

#include <pbdrv/battery.h>
#include <pbdrv/led.h>

#include "counter/counter.h"
#include "pwm/pwm.h"

/** Initializes all enabled drivers. */
void pbdrv_init() {
    clock_init();
    process_init();
    pbdrv_battery_init();
    pbdrv_counter_init();
    pbdrv_led_init();
    pbdrv_pwm_init();
}
