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
#include <pbio/light_animation.h>
#include <pbio/motor_process.h>
#include <pbio/port_interface.h>

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

/**
 * Stops all application-level background processes. Called when the user
 * application completes to get these modules back into their default state.
 * Drivers and OS-level processes continue running.
 *
 * @return   ::PBIO_SUCCESS when completed
 *           ::PBIO_ERROR_TIMEDOUT if it could not stop processes in a reasonable
 *             amount of time.
 */
pbio_error_t pbio_main_stop_application_resources(void) {

    pbio_main_soft_stop();

    pbio_error_t err;
    pbio_os_state_t state = 0;
    pbio_os_timer_t timer;
    pbio_os_timer_set(&timer, 5000);

    // Run event loop until Bluetooth is idle or times out.
    while ((err = pbdrv_bluetooth_close_user_tasks(&state, &timer)) == PBIO_ERROR_AGAIN) {
        pbio_os_run_processes_and_wait_for_event();
    }

    #if PBIO_CONFIG_LIGHT
    pbio_light_animation_stop_all();
    #endif

    #if PBDRV_CONFIG_DISPLAY
    pbio_image_fill(pbdrv_display_get_image(), 0);
    pbdrv_display_update();
    #endif

    pbio_os_run_processes_and_wait_for_event();

    return err;
}

/** @} */
