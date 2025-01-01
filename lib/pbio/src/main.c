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
#include <pbdrv/sound.h>
#include <pbio/config.h>
#include <pbio/dcmotor.h>
#include <pbio/imu.h>
#include <pbio/light_matrix.h>
#include <pbio/light.h>
#include <pbio/main.h>
#include <pbio/motor_process.h>
#include <pbio/port_interface.h>

#include "light/animation.h"

/**
 * Initialize the Pybricks I/O Library. This function must be called once,
 * usually at the beginning of a program, before using any other functions in
 * the library.
 *
 * @param [in]  start_processes  Whether to start all user-level background
 *                               processes. This is always enabled, except in
 *                               tests that test one driver at a time.
 */
void pbio_init(bool start_processes) {
    pbdrv_init();

    pbio_imu_init();

    if (!start_processes) {
        return;
    }

    // This will also initialize the dcmotor and servo pbio object instances.
    pbio_port_init();

    // Can the motor process after ports initialized above.
    pbio_motor_process_start();
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

    pbio_port_stop_user_actions(reset);

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
