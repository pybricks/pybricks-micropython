/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * \addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include "pbdrv/adc.h"
#include "pbdrv/bluetooth.h"
#include "pbdrv/button.h"
#include "pbdrv/battery.h"
#include "pbdrv/light.h"
#include "pbdrv/ioport.h"
#include "pbdrv/motor.h"
#include "pbdrv/uart.h"
#include "pbsys/sys.h"
#include "pbio/motorcontrol.h"
#include "pbio/uartdev.h"

#include "sys/autostart.h"
#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/process.h"

static uint32_t prev_fast_poll_time;
static uint32_t prev_slow_poll_time;

AUTOSTART_PROCESSES(
    &etimer_process
#if PBDRV_CONFIG_BLUETOOTH
    ,&pbdrv_bluetooth_hci_process
    ,&pbdrv_bluetooth_spi_process
#endif // PBDRV_CONFIG_BLUETOOTH
#if PBDRV_CONFIG_IOPORT
    ,&pbdrv_ioport_process
#endif
#if PBDRV_CONFIG_UART
    ,&pbdrv_uart_process
#endif
#ifndef PBIO_CONFIG_DISABLE_UARTDEV
    ,&pbio_uartdev_process
#endif
#ifdef PBIO_CONFIG_ENABLE_SYS
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
#ifdef PBIO_CONFIG_ENABLE_SYS
    process_init();
#endif
    _pbdrv_adc_init();
    _pbdrv_battery_init();
    _pbdrv_button_init();
    _pbdrv_light_init();
    _pbdrv_motor_init();
#ifdef PBIO_CONFIG_ENABLE_SYS
    autostart_start(autostart_processes);
#endif
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
        _pbdrv_adc_poll(now);
        _pbio_motorcontrol_poll();
        prev_fast_poll_time = now;
    }
    if (now - prev_slow_poll_time >= 32) {
        _pbio_light_poll(now);
        prev_slow_poll_time = now;
    }
#ifdef PBIO_CONFIG_ENABLE_SYS
    return process_run();
#else
    return 0;
#endif
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
/**
 * Releases all resources used by the library. Calling this function is
 * optional. It should be called once at the end of a program. No other
 * functions may be called after this.
 */
void pbio_deinit(void) {
#ifdef PBIO_CONFIG_ENABLE_SYS
    autostart_exit(autostart_processes);
#endif
    _pbdrv_motor_deinit();
    _pbdrv_light_deinit();
    _pbdrv_button_deinit();
    _pbdrv_battery_deinit();
    _pbdrv_adc_deinit();
}
#endif

/** @}*/
