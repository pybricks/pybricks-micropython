// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <stdbool.h>
#include <stddef.h>

#include <pbio/button.h>
#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/program_stop.h>

// Button combination that will trigger user program stop callback
static pbio_button_flags_t stop_buttons = PBIO_BUTTON_CENTER;
// State for button press one-shot
static bool stop_button_pressed;

/**
 * Request the user program to stop. For example, in MicroPython, this may raise
 * a SystemExit exception.
 */
void pbsys_program_stop(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        pbsys_main_stop_program();
    }
}

/**
 * Sets the stop button(s) to be used during a user program.
 * @param [in]  buttons     One or more button flags to set the stop button
 *                          or 0 to disable the stop button.
 */
void pbsys_program_stop_set_buttons(pbio_button_flags_t buttons) {
    stop_buttons = buttons;
}

/**
 * This is called periodically to monitor the user program.
 */
void pbsys_program_stop_poll(void) {

    // If shutdown was triggered, we need to stop even if the
    // stop button was disabled.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        pbsys_program_stop();
        return;
    }

    pbio_button_flags_t btn;
    pbio_button_is_pressed(&btn);

    if (!stop_buttons) {
        return;
    }

    if ((btn & stop_buttons) == stop_buttons) {
        if (!stop_button_pressed) {
            stop_button_pressed = true;
            pbsys_program_stop();
        }
    } else {
        stop_button_pressed = false;
    }
}
