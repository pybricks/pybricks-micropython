// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <contiki.h>

#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

#include "core.h"
#include "adc/adc.h"
#include "battery/battery.h"
#include "block_device/block_device.h"
#include "bluetooth/bluetooth.h"
#include "button/button.h"
#include "charger/charger.h"
#include "clock/clock.h"
#include "counter/counter.h"
#include "imu/imu.h"
#include "led/led_array.h"
#include "led/led.h"
#include "motor_driver/motor_driver.h"
#include "pwm/pwm.h"
#include "random/random.h"
#include "reset/reset.h"
#include "sound/sound.h"
#include "uart/uart.h"
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
    pbdrv_adc_init();
    pbdrv_battery_init();
    pbdrv_block_device_init();
    pbdrv_bluetooth_init();
    pbdrv_button_init();
    pbdrv_charger_init();
    pbdrv_counter_init();
    pbdrv_imu_init();
    pbdrv_led_array_init();
    pbdrv_led_init();
    pbdrv_motor_driver_init();
    pbdrv_pwm_init();
    pbdrv_random_init();
    pbdrv_reset_init();
    pbdrv_sound_init();
    pbdrv_uart_init();
    pbdrv_usb_init();
    pbdrv_watchdog_init();

    // Interrupts are disabled when transitioning from the bootloader to our
    // firmware. Enabling has to be done here otherwise Essential hub can hang
    // on boot if it is earlier.
    #if PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM
    __asm volatile ("cpsie i" : : : "memory");
    #endif

    // Some hubs turn on power to the I/O ports in the bootloader. This causes
    // us to miss the initial synchronization window, so we lose more time.
    // This is fixed by power cycling them here and allowing some reset time.
    #if PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE
    pbdrv_ioport_enable_vcc(false);
    uint32_t ioport_reset_time = pbdrv_clock_get_ms();
    #endif

    // Wait for all async pbdrv drivers to initialize before starting
    // higher level system processes.
    while (pbdrv_init_busy()) {
        process_run();
    }

    #if PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE
    while (pbdrv_clock_get_ms() - ioport_reset_time < 500) {
        process_run();
    }
    #endif
    pbdrv_ioport_enable_vcc(true);
}
