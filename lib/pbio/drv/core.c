// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

#include <contiki.h>

#include "core.h"
#include "battery/battery.h"
#include "block_device/block_device.h"
#include "bluetooth/bluetooth.h"
#include "charger/charger.h"
#include "clock/clock.h"
#include "counter/counter.h"
#include "imu/imu.h"
#include "ioport/ioport.h"
#include "led/led_array.h"
#include "led/led.h"
#include "motor_driver/motor_driver.h"
#include "pwm/pwm.h"
#include "reset/reset.h"
#include "sound/sound.h"
#include "usb/usb.h"
#include "watchdog/watchdog.h"

uint32_t pbdrv_init_busy_count;

/** Initializes all enabled drivers. */
void pbdrv_init(void) {
    // it is important that clocks go first since almost everything depends on clocks
    pbdrv_clock_init();
    process_init();
    process_start(&etimer_process);

    // the rest of the drivers should be implemented so that init order doesn't matter
    pbdrv_battery_init();
    pbdrv_block_device_init();
    pbdrv_bluetooth_init();
    pbdrv_charger_init();
    pbdrv_counter_init();
    pbdrv_imu_init();
    pbdrv_ioport_init();
    pbdrv_led_array_init();
    pbdrv_led_init();
    pbdrv_motor_driver_init();
    pbdrv_pwm_init();
    pbdrv_reset_init();
    pbdrv_sound_init();
    pbdrv_usb_init();
    pbdrv_watchdog_init();

    while (pbdrv_init_busy()) {
        process_run();
    }
}
