// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

/**
 * @addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <contiki.h>

#include <pbdrv/button.h>
#include <pbdrv/config.h>
#include <pbdrv/core.h>
#include <pbdrv/ioport.h>
#include <pbdrv/sound.h>
#include <pbio/config.h>
#include <pbio/dcmotor.h>
#include <pbio/light_matrix.h>
#include <pbio/light.h>
#include <pbio/main.h>
#include <pbio/uartdev.h>

#include "light/animation.h"
#include "processes.h"

// DO NOT ADD NEW PROCESSES HERE!
// We are trying to remove the use of autostart.
AUTOSTART_PROCESSES(
#if PBDRV_CONFIG_ADC
    &pbdrv_adc_process,
#endif
#if PBDRV_CONFIG_UART
    &pbdrv_uart_process,
#endif
#if PBIO_CONFIG_UARTDEV
    &pbio_uartdev_process,
#endif
#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0
    &pbio_motor_process,
#endif
    NULL);

/**
 * Initialize the Pybricks I/O Library. This function must be called once,
 * usually at the beginning of a program, before using any other functions in
 * the library.
 */
void pbio_init(void) {
    pbdrv_init();
    autostart_start(autostart_processes);
}

/**
 * Stops all user-level background processes. Drivers and OS-level processes
 * continue running.
 *
 * @param [in]  reset  Whether to reset all user-level processes to a clean
 *                     state (true), or whether to only stop active outputs
 *                     like sound or motors (false). The latter is useful
 *                     to preserve the state for debugging, without sound
 *                     or movement getting in the way or out of control.
 */
void pbio_stop_all(bool reset) {
    #if PBIO_CONFIG_LIGHT
    if (reset) {
        pbio_light_animation_stop_all();
    }
    #endif
    pbio_dcmotor_stop_all(reset);
    pbdrv_sound_stop();
}

/**
 * Checks for and performs pending background tasks.
 *
 * This function is meant to be called as frequently as possible. To conserve
 * power, you can wait for an interrupt after all events have been processed
 * (i.e. return value is 0).
 *
 * Important!!! This function must not be called recursively.
 *
 * @return      The number of still-pending events.
 */
int pbio_do_one_event(void) {
    return process_run();
}

/** @} */
