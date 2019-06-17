// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

/**
 * \addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include "pbdrv/button.h"
#include "pbdrv/config.h"
#include "pbdrv/light.h"
#include "pbdrv/motor.h"
#include "pbsys/sys.h"
#include "pbio/config.h"
#include "pbio/motor.h"
#include "pbio/uartdev.h"

#include "sys/autostart.h"
#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include "processes.h"

static uint32_t prev_fast_poll_time;
static uint32_t prev_slow_poll_time;

AUTOSTART_PROCESSES(
    &etimer_process
#if PBDRV_CONFIG_ADC
    ,&pbdrv_adc_process
#endif
#if PBDRV_CONFIG_BATTERY
    ,&pbdrv_battery_process
#endif
#if PBDRV_CONFIG_BLUETOOTH
    ,&pbdrv_bluetooth_hci_process
    ,&pbdrv_bluetooth_spi_process
#endif
#if PBDRV_CONFIG_IOPORT_LPF2
    ,&pbdrv_ioport_lpf2_process
#endif
#if PBDRV_CONFIG_UART
    ,&pbdrv_uart_process
#endif
#if PBIO_CONFIG_UARTDEV
    ,&pbio_uartdev_process
#endif
#if PBIO_CONFIG_ENABLE_SYS
    ,&pbsys_process
#endif
);

/**
 * Initialize the Pybricks I/O Library. This function must be called once,
 * usually at the beginning of a program, before using any other functions in
 * the library.
 */
void pbio_init(void) {
    clock_init();
    process_init();
    _pbdrv_button_init();
    _pbdrv_light_init();
    _pbdrv_motor_init();
    _pbio_motorcontroll_init();
    autostart_start(autostart_processes);
}

/**
 * Checks for and performs pending background tasks. This function is meant to
 * be called as frequently as possible. To conserve power, you an wait for an
 * interrupt after all events have been processed (i.e. return value is 0).
 * @return      The number of still-pending events.
 */
int pbio_do_one_event(void) {
    clock_time_t now = clock_time();

    // pbio_do_one_event() can be called quite frequently (e.g. in a tight loop) so we
    // don't want to call all of the subroutines unless enough time has
    // actually elapsed to do something useful.
    if (now - prev_fast_poll_time >= 2) {
        _pbio_motorcontrol_poll();
        prev_fast_poll_time = now;
    }
    if (now - prev_slow_poll_time >= 32) {
        _pbio_light_poll(now);
        prev_slow_poll_time = now;
    }
    return process_run();
}

#if PBIO_CONFIG_ENABLE_DEINIT
/**
 * Releases all resources used by the library. Calling this function is
 * optional. It should be called once at the end of a program. No other
 * functions may be called after this.
 */
void pbio_deinit(void) {
    autostart_exit(autostart_processes);
    _pbdrv_motor_deinit();
    _pbdrv_light_deinit();
    _pbdrv_button_deinit();
}
#endif

/** @}*/
