// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <contiki.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_INIT_NXOS
#include <nxos/_display.h>
#include <nxos/interrupts.h>
#include <nxos/assert.h>
#include <nxos/drivers/_aic.h>
#include <nxos/drivers/_avr.h>
#include <nxos/drivers/_motors.h>
#include <nxos/drivers/_lcd.h>
#include <nxos/drivers/_sensors.h>
#include <nxos/drivers/_usb.h>
#include <nxos/drivers/i2c.h>
#include <nxos/drivers/systick.h>
#endif

#include "core.h"
#include "battery/battery.h"
#include "block_device/block_device.h"
#include "bluetooth/bluetooth.h"
#include "button/button.h"
#include "charger/charger.h"
#include "clock/clock.h"
#include "counter/counter.h"
#include "imu/imu.h"
#include "ioport/ioport.h"
#include "led/led_array.h"
#include "led/led.h"
#include "motor_driver/motor_driver.h"
#include "pwm/pwm.h"
#include "random/random.h"
#include "reset/reset.h"
#include "sound/sound.h"
#include "usb/usb.h"
#include "watchdog/watchdog.h"

uint32_t pbdrv_init_busy_count;

/** Initializes all enabled drivers. */
void pbdrv_init(void) {
    #if PBDRV_CONFIG_INIT_NXOS
    nx__aic_init();
    // TODO: can probably move nx_interrupts_enable() down with
    // PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM after nx_systick_wait_ms()
    // is removed
    nx_interrupts_enable(0);
    #endif
    // it is important that clocks go first since almost everything depends on clocks
    pbdrv_clock_init();
    process_init();
    process_start(&etimer_process);

    // TODO: we should be able to convert these to generic pbio drivers and use
    // pbdrv_init_busy instead of busy waiting for 100ms.
    #if PBDRV_CONFIG_INIT_NXOS
    nx__avr_init();
    nx__motors_init();
    nx__lcd_init();
    nx__display_init();
    nx__sensors_init();
    nx__usb_init();
    nx_i2c_init();

    /* Delay a little post-init, to let all the drivers settle down. */
    nx_systick_wait_ms(100);
    #endif

    // the rest of the drivers should be implemented so that init order doesn't matter
    pbdrv_battery_init();
    pbdrv_block_device_init();
    pbdrv_bluetooth_init();
    pbdrv_button_init();
    pbdrv_charger_init();
    pbdrv_counter_init();
    pbdrv_imu_init();
    pbdrv_ioport_init();
    pbdrv_led_array_init();
    pbdrv_led_init();
    pbdrv_motor_driver_init();
    pbdrv_pwm_init();
    pbdrv_random_init();
    pbdrv_reset_init();
    pbdrv_sound_init();
    pbdrv_usb_init();
    pbdrv_watchdog_init();

    // Some hubs have a bootloader that disables interrupts. Has to be done
    // here otherwise Essential hub can hang on boot if it is earlier.
    #if PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM
    __asm volatile ("cpsie i" : : : "memory");
    #endif

    while (pbdrv_init_busy()) {
        process_run();
    }
}
