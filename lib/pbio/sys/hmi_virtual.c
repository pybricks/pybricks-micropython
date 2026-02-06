// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Drop-in for HMI module that simulates sending a multi-mpy file to the hub
// and then starts it. Used in the simulated hub to test different programs.
// The script to start is set with an environment variable.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_VIRTUAL

#include <pbdrv/display.h>

#include <pbio/button.h>
#include <pbio/image.h>
#include <pbio/os.h>
#include <pbsys/command.h>
#include <pbsys/host.h>
#include <pbsys/main.h>
#include <pbsys/status.h>

#include "storage.h"
#include "hmi.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

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

static void hmi_lcd_grid_show_pixel(uint8_t row, uint8_t col, bool on) {
    pbio_image_t *display = pbdrv_display_get_image();
    uint8_t value = on ? pbdrv_display_get_max_value(): 0;
    const uint32_t size = PBDRV_CONFIG_DISPLAY_NUM_ROWS / PBSYS_CONFIG_HMI_NUM_SLOTS;
    const uint32_t width = size * 4 / 5;
    const uint32_t offset = (PBDRV_CONFIG_DISPLAY_NUM_COLS - (PBSYS_CONFIG_HMI_NUM_SLOTS * size)) / 2;
    pbio_image_fill_rect(display, col * size + offset, row * size, width, width, value);
}

void pbsys_hmi_deinit(void) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, 0);
    pbdrv_display_update();
}

static pbio_error_t run_ui(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        DEBUG_PRINT("Start HMI loop\n");

        // Visually indicate current slot.
        #if PBSYS_CONFIG_HMI_NUM_SLOTS
        uint8_t selected_slot = pbsys_status_get_selected_slot();
        for (uint8_t c = 0; c < PBSYS_CONFIG_HMI_NUM_SLOTS; c++) {
            hmi_lcd_grid_show_pixel(4, c, c == selected_slot);
        }
        #endif

        pbdrv_display_update();

        // Buttons could be pressed at the end of the user program, so wait for
        // a release and then a new press, or until we have to exit early.
        DEBUG_PRINT("Waiting for initial button release.\n");
        PBIO_OS_AWAIT_WHILE(state, ({
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                return PBIO_ERROR_CANCELED;
            }
            pbdrv_button_get_pressed();
        }));

        DEBUG_PRINT("Start waiting for input.\n");
        // Wait on a button, external program start, or connection change. Stop
        // waiting on timeout or shutdown.
        PBIO_OS_AWAIT_UNTIL(state, ({
            // Shutdown may be requested by a background process such as critical
            // battery or holding the power button.
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                return PBIO_ERROR_CANCELED;
            }

            // Exit on timeout except while connected to host.
            if (pbsys_host_is_connected()) {
                pbio_os_timer_reset(timer);
            } else if (pbio_os_timer_is_expired(timer)) {
                return PBIO_ERROR_TIMEDOUT;
            }

            // Wait for button press, external program start, or connection change.
            pbdrv_button_get_pressed() || pbsys_main_program_start_is_requested();
        }));

        // External progran request takes precedence over buttons.
        if (pbsys_main_program_start_is_requested()) {
            DEBUG_PRINT("Start program from Pybricks Code.\n");
            break;
        }

        // On right, increment slot when possible, then start waiting on new inputs.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_RIGHT) {
            pbsys_status_increment_selected_slot(true);
            continue;
        }
        // On left, decrement slot when possible, then start waiting on new inputs.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_LEFT) {
            pbsys_status_increment_selected_slot(false);
            continue;
        }

        // On center, attempt to start program.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER) {
            pbio_error_t err = pbsys_main_program_request_start(pbsys_status_get_selected_slot(), PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);
            if (err == PBIO_SUCCESS) {
                DEBUG_PRINT("Start program with button\n");
                break;
            } else {
                DEBUG_PRINT("Requested program not available.\n");
                // We can run an animation here to indicate that the program is not available.
            }
        }

        DEBUG_PRINT("No valid action selected, start over.\n");
    }

    // Wait for all buttons to be released so the user doesn't accidentally
    // push their robot off course.
    DEBUG_PRINT("Waiting for final button release.\n");
    PBIO_OS_AWAIT_WHILE(state, ({
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }
        pbdrv_button_get_pressed();
    }));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


/**
 * Interactive menu similar to real embedded hubs.
 */
static pbio_error_t pbsys_hmi_await_program_selection_interactive(void) {

    pbio_os_timer_t idle_timer;
    pbio_os_timer_set(&idle_timer, PBSYS_CONFIG_HMI_IDLE_TIMEOUT_MS);

    pbio_os_state_t state = 0;

    pbio_error_t err;
    while ((err = run_ui(&state, &idle_timer)) == PBIO_ERROR_AGAIN) {
        // run all processes and wait for next event.
        pbio_os_run_processes_and_wait_for_event();
    }
    DEBUG_PRINT("Finished program selection with status: %d\n", err);
    return err;
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
