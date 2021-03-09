// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <stddef.h>

#include <pbio/main.h>
#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>

// user program stop function
static pbsys_user_program_stop_t user_stop_func;

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
