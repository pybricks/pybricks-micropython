// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_USER_PROGRAM

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/block_device.h>
#include <pbio/main.h>
#include <pbio/protocol.h>
#include <pbsys/main.h>
#include <pbsys/status.h>

#include "core.h"
#include "storage.h"

static bool pbsys_user_program_start_user_program_requested;
static bool pbsys_user_program_start_repl_requested;

/**
 * Requests to start the user program.
 *
 * @returns     ::PBIO_ERROR_BUSY if a user program is already running.
 *              ::PBIO_ERROR_INVALID_ARG if the user program has an invalid size.
 *              ::PBIO_ERROR_NOT_SUPPORTED if the program load module is disabled.
 *              Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_user_program_start_program(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    pbio_error_t err = pbsys_storage_assert_program_valid();
    if (err != PBIO_SUCCESS) {
        return err;
    }

    pbsys_user_program_start_user_program_requested = true;

    return PBIO_SUCCESS;
}

/**
 * Requests to start the REPL.
 *
 * @returns     ::PBIO_ERROR_BUSY if a user program is already running.
 *              ::PBIO_ERROR_NOT_SUPPORTED if the program load module is disabled.
 *              Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_user_program_start_repl(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    pbsys_user_program_start_repl_requested = true;

    return PBIO_SUCCESS;
}

/**
 * Waits for a command to start a user program or REPL.
 *
 * NOTE: this function runs the contiki event loop, so it should not be called
 * from inside an contiki process.
 *
 * @param [out] program         Program info structure to be populated.
 * @return                      ::PBIO_SUCCESS on success.
 *                              ::PBIO_ERROR_CANCELED when canceled due to shutdown request.
 *                              ::PBIO_ERROR_NOT_SUPPORTED if the program load module is disabled.
 */
pbio_error_t pbsys_user_program_wait_command(pbsys_main_program_t *program) {
    for (;;) {
        // REVISIT: this can be long waiting, so we could do a more efficient
        // wait (i.e. __WFI() on embedded system)
        pbio_do_one_event();

        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }

        #if PBSYS_CONFIG_USER_PROGRAM_AUTO_START
        pbsys_user_program_start_user_program_requested = true;
        #endif

        if (pbsys_user_program_start_user_program_requested) {
            pbsys_user_program_start_user_program_requested = false;
            program->run_builtin = false;
            break;
        }

        if (pbsys_user_program_start_repl_requested) {
            pbsys_user_program_start_repl_requested = false;
            program->run_builtin = true;
            break;
        }
    }

    // Builtin program can also use user program (e.g. in MicroPython, REPL may
    // import user modules), so load data in all cases.
    pbsys_storage_get_program_data(program);

    return PBIO_SUCCESS;
}

#endif // PBSYS_CONFIG_USER_PROGRAM
