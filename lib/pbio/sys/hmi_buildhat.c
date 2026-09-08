// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

// Provides the Human Machine Interface (HMI) for the Build HAT.
// There is nothing to select, so the REPL starts right after boot.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_BUILDHAT

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbio/light.h>
#include <pbsys/light.h>
#include <pbsys/main.h>

void pbsys_hmi_init(void) {
}

void pbsys_hmi_deinit(void) {
}

void pbsys_hmi_stop_animation(void) {
}

void pbsys_hmi_connection_changed_handler(void) {
}

pbio_error_t pbsys_hmi_await_program_selection(void) {

    // Start the "ready" animation, same as hmi_pup.c does after program
    // selection.
    #if PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS
    pbio_color_light_start_breathe_animation(pbsys_status_light_main, PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS_HUE);
    #elif PBSYS_CONFIG_STATUS_LIGHT
    pbio_color_light_off(pbsys_status_light_main);
    #endif

    return pbsys_main_program_request_start(PBIO_PYBRICKS_USER_PROGRAM_ID_REPL, PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_BOOT);
}

#endif // PBSYS_CONFIG_HMI_BUILDHAT
