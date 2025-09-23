// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_STOP

#include <stdbool.h>
#include <stddef.h>

#include <pbio/button.h>
#include <pbio/main.h>

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
 *
 * @param [in]  force_stop  Whether to force stop the program instead of asking
 *                          nicely. This is true when the application must stop
 *                          on shutdown.
 */
void pbsys_program_stop(bool force_stop) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        pbsys_main_stop_program(force_stop);
    }
}

/**
 * Gets the stop button(s) used during a user program.
 * @param [in]  buttons     One or more button flags that stop the program
 *                          or 0 if stopping by button is disabled.
 */
pbio_button_flags_t pbsys_program_stop_get_buttons(void) {
    return stop_buttons;
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
 * This is called periodically to monitor the power button and shutdown requests.
 */
void pbsys_program_stop_poll(void) {

    pbio_button_flags_t btn = pbdrv_button_get_pressed();

    if (btn & PBIO_BUTTON_CENTER) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);

        // power off when button is held down for 2 seconds
        if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED, true, 2000)) {
            pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
        }
    } else {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_POWER_BUTTON_PRESSED);
    }

    // Cancel user application program if shutdown was requested.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
        pbsys_program_stop(true);
        return;
    }

    if (!stop_buttons) {
        return;
    }

    if ((btn & stop_buttons) == stop_buttons) {
        if (!stop_button_pressed) {
            stop_button_pressed = true;
            pbsys_program_stop(false);
        }
    } else {
        stop_button_pressed = false;
    }
}

#endif // PBSYS_CONFIG_PROGRAM_STOP
