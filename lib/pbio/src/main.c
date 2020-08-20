// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <contiki.h>

#include "pbdrv/button.h"
#include "pbdrv/config.h"
#include "pbdrv/core.h"
#include "pbdrv/motor.h"
#include "pbsys/sys.h"
#include "pbio/config.h"
#include "pbio/light.h"
#include "pbio/lightgrid.h"
#include "pbio/motorpoll.h"
#include "pbio/uartdev.h"

#include "processes.h"

static clock_time_t prev_fast_poll_time;
static clock_time_t prev_slow_poll_time;

AUTOSTART_PROCESSES(
    &etimer_process
#if PBDRV_CONFIG_ADC
    ,&pbdrv_adc_process
#endif
#if PBDRV_CONFIG_BLUETOOTH
    ,&pbdrv_bluetooth_hci_process
    ,&pbdrv_bluetooth_spi_process
#endif
#if PBDRV_CONFIG_COUNTER
    ,&pbdrv_counter_process
#endif
#if PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH
    ,&pbdrv_ioport_ev3dev_stretch_process
#endif
#if PBDRV_CONFIG_IOPORT_LPF2
    ,&pbdrv_ioport_lpf2_process
#endif
#if PBDRV_CONFIG_UART
    ,&pbdrv_uart_process
#endif
#if PBDRV_CONFIG_USB
    ,&pbdrv_usb_process
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
    pbdrv_init();
    _pbdrv_button_init();
    autostart_start(autostart_processes);
    _pbio_motorpoll_reset_all();
}

/**
 * Checks for and performs pending background tasks. This function is meant to
 * be called as frequently as possible. To conserve power, you can wait for an
 * interrupt after all events have been processed (i.e. return value is 0).
 * @return      The number of still-pending events.
 */
int pbio_do_one_event(void) {
    clock_time_t now = clock_time();

    // pbio_do_one_event() can be called quite frequently (e.g. in a tight loop) so we
    // don't want to call all of the subroutines unless enough time has
    // actually elapsed to do something useful.
    if (now - prev_fast_poll_time >= clock_from_msec(PBIO_CONFIG_SERVO_PERIOD_MS)) {
        _pbio_motorpoll_poll();
        prev_fast_poll_time = clock_time();
    }
    if (now - prev_slow_poll_time >= clock_from_msec(32)) {
        _pbio_light_poll(now);
        prev_slow_poll_time = now;
    }
    return process_run();
}

/** @}*/
