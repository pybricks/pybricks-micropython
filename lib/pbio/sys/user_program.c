// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <stdbool.h>
#include <stddef.h>

#include <pbio/button.h>
#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>

// user program stop function
static pbsys_user_program_stop_t user_stop_func;

// Button combination that will trigger user program stop callback
static pbio_button_flags_t stop_buttons = PBIO_BUTTON_CENTER;
// State for button press one-shot
static bool stop_button_pressed;

/**
 * Prepares the runtime for running a user program.
 * @param [in]  callbacks   Optional struct of callback function pointers.
 */
void pbsys_user_program_prepare(const pbsys_user_program_callbacks_t *callbacks) {
    if (callbacks) {
        user_stop_func = callbacks->stop;
        pbsys_bluetooth_rx_set_callback(callbacks->stdin_event);
    } else {
        user_stop_func = NULL;
        pbsys_bluetooth_rx_set_callback(NULL);
    }
    pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
}

/**
 * Performs cleanup/reset after running a user program.
 */
void pbsys_user_program_unprepare(void) {
    pbsys_status_clear(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
    pbio_stop_all();
    user_stop_func = NULL;
    stop_buttons = PBIO_BUTTON_CENTER;
    pbsys_bluetooth_rx_set_callback(NULL);
}

/**
 * Request the user program to stop. For example, in MicroPython, this may raise
 * a SystemExit exception.
 */
void pbsys_user_program_stop(void) {
    if (user_stop_func) {
        user_stop_func();
    }
}

/**
 * Sets the stop button(s) to be used during a user program.
 * @param [in]  buttons     One or more button flags to set the stop button
 *                          or 0 to disable the stop button.
 */
void pbsys_user_program_set_stop_buttons(pbio_button_flags_t buttons) {
    stop_buttons = buttons;
}

/**
 * This is called periodically to monitor the user program.
 */
void pbsys_user_program_poll(void) {
    pbio_button_flags_t btn;
    pbio_button_is_pressed(&btn);

    if (!stop_buttons) {
        return;
    }

    if ((btn & stop_buttons) == stop_buttons) {
        if (!stop_button_pressed) {
            stop_button_pressed = true;
            pbsys_user_program_stop();
        }
    } else {
        stop_button_pressed = false;
    }
}
