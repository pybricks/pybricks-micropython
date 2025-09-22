// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

/**
 * @addtogroup Main Library initialization and events
 * @{
 */

#include <stdbool.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/display.h>
#include <pbdrv/sound.h>

#include <pbio/battery.h>
#include <pbio/image.h>
#include <pbio/imu.h>
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

    pbio_battery_init();
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
 * Deinitialize pbio modules that are not needed after soft-poweroff.
 */
void pbio_deinit(void) {
    // Power off sensors and motors, including the ones that are always powered.
    pbio_port_power_off();
}

/**
 * Stops resources like motors or sounds or peripheral procedures that take a
 * long time.
 *
 * Useful to get the system in a safe state for the user without doing a full
 * reset. Applications can all this to enter a user debug mode like the
 * MicroPython REPL.
 */
void pbio_main_soft_stop(void) {

    pbio_port_stop_user_actions(false);

    pbdrv_sound_stop();

    pbdrv_bluetooth_cancel_operation_request();
}

static void wait_for_bluetooth(void) {
    pbio_os_state_t unused;
    while (pbdrv_bluetooth_await_advertise_or_scan_command(&unused, NULL) == PBIO_ERROR_AGAIN ||
           pbdrv_bluetooth_await_peripheral_command(&unused, NULL) == PBIO_ERROR_AGAIN) {

        // Run event loop until Bluetooth is idle.
        pbio_os_run_processes_and_wait_for_event();
    }
}

/**
 * Stops all application-level background processes. Called when the user
 * application completes to get these modules back into their default state.
 * Drivers and OS-level processes continue running.
 */
void pbio_main_stop_application_resources() {

    pbio_main_soft_stop();

    // Let ongoing task finish first.
    wait_for_bluetooth();

    // Stop broadcasting, observing and disconnect peripheral.
    pbdrv_bluetooth_start_broadcasting(NULL, 0);
    wait_for_bluetooth();

    pbdrv_bluetooth_start_observing(NULL);
    wait_for_bluetooth();

    pbdrv_bluetooth_peripheral_disconnect();
    wait_for_bluetooth();

    #if PBIO_CONFIG_LIGHT
    pbio_light_animation_stop_all();
    #endif

    #if PBDRV_CONFIG_DISPLAY
    pbio_image_fill(pbdrv_display_get_image(), 0);
    pbdrv_display_update();
    #endif
}

/** @} */
