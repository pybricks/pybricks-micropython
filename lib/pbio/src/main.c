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
#include <pbdrv/motor.h>
#include <pbdrv/sound.h>
#include <pbio/config.h>
#include <pbio/light_matrix.h>
#include <pbio/light.h>
#include <pbio/main.h>
#include <pbio/motor_process.h>
#include <pbio/uartdev.h>

#include "light/animation.h"
#include "processes.h"

AUTOSTART_PROCESSES(
#if PBDRV_CONFIG_ADC
    &pbdrv_adc_process,
#endif
#if PBDRV_CONFIG_UART
    &pbdrv_uart_process,
#endif
#if PBDRV_CONFIG_USB
    &pbdrv_usb_process,
#endif
#if PBIO_CONFIG_UARTDEV
    &pbio_uartdev_process,
#endif
#if PBIO_CONFIG_ENABLE_SYS
    &pbsys_process,
#endif
#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0
    &pbio_motor_process,
#endif
    NULL);

static pbio_event_hook_t pbio_event_hook;

/**
 * Initialize the Pybricks I/O Library. This function must be called once,
 * usually at the beginning of a program, before using any other functions in
 * the library.
 */
void pbio_init(void) {
    pbdrv_init();
    _pbdrv_button_init();
    autostart_start(autostart_processes);
}

/**
 * Stops all user-level background processes. Drivers and OS-level processes
 * continue running.
 */
void pbio_stop_all(void) {
    #if PBIO_CONFIG_LIGHT
    pbio_light_animation_stop_all();
    #endif
    #if PBDRV_CONFIG_IOPORT_LPF2
    pbdrv_ioport_reset_passive_devices();
    #endif
    pbio_motor_process_reset();
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
    if (pbio_event_hook) {
        pbio_event_hook();
    }
    return process_run();
}

/**
 * Sets a callback that is called each time pbio_do_one_event() is called.
 *
 * @param [in]  hook        A callback function or NULL.
 */
void pbio_set_event_hook(pbio_event_hook_t hook) {
    pbio_event_hook = hook;
}

/** @} */
