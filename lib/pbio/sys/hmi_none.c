// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user for Powered Up
// hubs with BLE, lights, and one or more buttons.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_NONE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbio/button.h>
#include <pbio/os.h>
#include <pbsys/main.h>
#include <pbsys/status.h>

pbio_error_t pbsys_hmi_await_program_selection(void) {

    while (pbdrv_button_get_pressed()) {
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }
        pbio_os_run_processes_and_wait_for_event();
    }

    return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_REPL, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
}

#endif // PBSYS_CONFIG_HMI_NONE
