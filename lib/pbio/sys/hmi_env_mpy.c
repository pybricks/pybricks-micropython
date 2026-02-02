// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Drop-in for HMI module that simulates sending a multi-mpy file to the hub
// and then starts it. Used in the simulated hub to test different programs.
// The script to start is set with an environment variable.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_ENV_MPY

#include <pbio/button.h>
#include <pbio/os.h>
#include <pbsys/command.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include "storage.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void pbsys_hmi_init(void) {
}

void pbsys_hmi_deinit(void) {
}

uint8_t pbsys_hmi_native_program_buf[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
uint32_t pbsys_hmi_native_program_size;

static uint32_t pbsys_hmi_native_program_count;

pbio_error_t pbsys_hmi_await_program_selection(void) {

    pbio_os_run_processes_and_wait_for_event();

    // With this HMI, we run a script several times and then exit.
    if (pbsys_hmi_native_program_count++ >= PBSYS_CONFIG_HMI_ENV_MPY_NUM_RUNS) {
        return PBIO_ERROR_CANCELED;
    }

    // Start REPL if no program given.
    if (pbsys_hmi_native_program_size == 0) {
        return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_REPL, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
    }

    // Load the program in storage, as if receiving it.
    pbsys_storage_set_program_size(0);
    pbsys_storage_set_program_data(0, pbsys_hmi_native_program_buf, pbsys_hmi_native_program_size);
    pbsys_storage_set_program_size(pbsys_hmi_native_program_size);

    return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_FIRST_SLOT, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
}

#endif // PBSYS_CONFIG_HMI_ENV_MPY
