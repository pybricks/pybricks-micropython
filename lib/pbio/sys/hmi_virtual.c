// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Drop-in for HMI module that simulates sending a multi-mpy file to the hub
// and then starts it. Used in the simulated hub to test different programs.
// The script to start is set with an environment variable.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_VIRTUAL

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
#include <string.h>
#include <sys/stat.h>

static uint32_t pbsys_hmi_num_programs;

static void load_program(const char *path) {
    // Pybricksdev helper script, pipes multi-mpy to us.
    char command[512];
    snprintf(command, sizeof(command), "pybricksdev compile --bin %s", path);
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        printf("Failed to compile program with Pybricksdev\n");
        exit(0);
    }

    // Read the multi-mpy file from pipe.
    uint8_t program_buf[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
    uint32_t program_size;
    program_size = fread(program_buf, 1, sizeof(program_buf), pipe);
    pclose(pipe);

    if (program_size == 0) {
        printf("Error reading from pipe\n");
        exit(0);
    }

    // Load the program in storage, as if receiving it.
    pbsys_storage_set_program_size(0);
    pbsys_storage_set_program_data(0, program_buf, program_size);
    pbsys_storage_set_program_size(program_size);
}

void pbsys_hmi_init(void) {
    extern int main_argc;
    extern char **main_argv;

    // Parse given programs.
    for (int i = 1; i < main_argc; i++) {

        size_t len = strlen(main_argv[i]);
        if (len < 3 || strcmp(main_argv[i] + len - 3, ".py")) {
            continue;
        }

        // Load program into next slot, as if downloaded externally.
        load_program(main_argv[i]);
        pbsys_status_increment_selected_slot(true);
        pbsys_hmi_num_programs++;
    }

    // Start at zero.
    for (int i = 0; i < pbsys_hmi_num_programs; i++) {
        pbsys_status_increment_selected_slot(false);
    }
}

void pbsys_hmi_deinit(void) {
}


/**
 * Runs one program (or REPL if nothing given) and shuts down the hub.
 */
static pbio_error_t pbsys_hmi_await_program_selection_one_off(void) {

    pbio_os_run_processes_and_wait_for_event();

    // With this HMI, we run a script once and then exit.
    static bool ran_once = false;
    if (ran_once) {
        return PBIO_ERROR_CANCELED;
    }
    ran_once = true;

    pbio_pybricks_user_program_id_t id = pbsys_hmi_num_programs ?
        PBIO_PYBRICKS_USER_PROGRAM_ID_FIRST_SLOT :
        PBIO_PYBRICKS_USER_PROGRAM_ID_REPL;

    return pbsys_main_program_request_start(id, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
}

/**
 * Interactive menu similar to real embedded hubs.
 */
static pbio_error_t pbsys_hmi_await_program_selection_interactive(void) {

    pbio_os_run_processes_and_wait_for_event();

    // TODO: Implement interactive menu.
    return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_FIRST_SLOT, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
}

/**
 * Run the one-off script and exit if zero or one scripts given, otherwise
 * run interactive menu. Press numpad keys in the animation to operate it.
 */
pbio_error_t pbsys_hmi_await_program_selection(void) {
    return pbsys_hmi_num_programs <= 1 ?
           pbsys_hmi_await_program_selection_one_off() :
           pbsys_hmi_await_program_selection_interactive();
}

#endif // PBSYS_CONFIG_HMI_VIRTUAL
