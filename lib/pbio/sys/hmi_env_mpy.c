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

pbio_error_t pbsys_hmi_await_program_selection(void) {

    pbio_os_run_processes_and_wait_for_event();

    // With this HMI, we run a script once and then exit.
    static bool ran_once = false;
    if (ran_once) {
        return PBIO_ERROR_CANCELED;
    }
    ran_once = true;

    // Test if script is provided via environment.
    const char *script_path = getenv("TEST_SCRIPT");
    if (!script_path) {
        // No script given, just start REPL
        return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_REPL, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
    }

    // Pybricksdev helper script, pipes multi-mpy to us.
    char command[512];
    snprintf(command, sizeof(command), "python ./bricks/simhub/make_mpy.py %s", script_path);
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        perror("Failed to compile program with Pybricksdev\n");
        return PBIO_ERROR_CANCELED;
    }

    // Read the multi-mpy file from pipe.
    uint8_t program_buf[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
    size_t read_size = fread(program_buf, 1, sizeof(program_buf), pipe);
    pclose(pipe);

    if (read_size == 0) {
        perror("Error reading from pipe");
        return PBIO_ERROR_CANCELED;
    }

    // Load the program in storage, as if receiving it.
    pbsys_storage_set_program_size(0);
    pbsys_storage_set_program_data(0, program_buf, read_size);
    pbsys_storage_set_program_size(read_size);

    return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_FIRST_SLOT, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
}

#endif // PBSYS_CONFIG_HMI_ENV_MPY
