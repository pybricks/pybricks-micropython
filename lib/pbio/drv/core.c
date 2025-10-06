// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <contiki.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/config.h>
#include <pbdrv/ioport.h>

#include <pbio/busy_count.h>
#include <pbio/os.h>

#include "adc/adc.h"
#include "battery/battery.h"
#include "block_device/block_device.h"
#include "bluetooth/bluetooth.h"
#include "button/button.h"
#include "charger/charger.h"
#include "clock/clock.h"
#include "counter/counter.h"
#include "display/display.h"
#include "i2c/i2c.h"
#include "imu/imu.h"
#include "led/led_array.h"
#include "led/led.h"
#include "motor_driver/motor_driver.h"
#include "pwm/pwm.h"
#include "random/random.h"
#include "reset/reset.h"
#include "rproc/rproc.h"
#include "sound/sound.h"
#include "uart/uart.h"
#include "uart/uart_debug_first_port.h"
#include "usb/usb.h"
#include "watchdog/watchdog.h"

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
    pbdrv_display_init();
    pbdrv_i2c_init();
    pbdrv_imu_init();
    pbdrv_led_array_init();
    pbdrv_led_init();
    pbdrv_motor_driver_init();
    pbdrv_pwm_init();
    pbdrv_random_init();
    pbdrv_reset_init();
    pbdrv_rproc_init();
    pbdrv_sound_init();
    pbdrv_uart_init();
    pbdrv_uart_debug_init();
    pbdrv_usb_init();
    pbdrv_watchdog_init();

    // Interrupts are disabled when transitioning from the bootloader to our
    // firmware. Enabling has to be done here otherwise Essential hub can hang
    // on boot if it is earlier.
    #if PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM
    __asm volatile ("cpsie i" : : : "memory");
    #endif

    // Wait for all async pbdrv drivers to initialize before starting
    // higher level system processes.
    while (pbio_busy_count_busy()) {
        pbio_os_run_processes_once();
    }

    pbdrv_ioport_enable_vcc(true);
}

/**
 * Deinitializes selected drivers that are not needed after soft-shutdown.
 */
void pbdrv_deinit(void) {

    pbdrv_imu_deinit();
    pbdrv_bluetooth_deinit();

    while (pbio_busy_count_busy()) {
        pbio_os_run_processes_once();
    }

}
