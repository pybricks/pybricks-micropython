// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_HMI_EV3_UI_H_
#define _PBSYS_SYS_HMI_EV3_UI_H_

#include <pbio/button.h>
#include <pbio/os.h>

typedef enum {
    /**
     * No action required.
     */
    PBSYS_HMI_EV3_UI_ACTION_NONE,
    /**
     * No action required, but refresh the UI again soon. Can be used to
     * update Bluetooth scan result UIs for example.
     */
    PBSYS_HMI_EV3_UI_ACTION_REFRESH_SOON,
    /**
     * Sets active code slot (payload: slot id).
     */
    PBSYS_HMI_EV3_UI_ACTION_SET_SLOT,
    /**
     * Run a program (payload: program id).
     */
    PBSYS_HMI_EV3_UI_ACTION_RUN_PROGRAM,
    /**
     * Shut the hub down.
     */
    PBSYS_HMI_EV3_UI_ACTION_SHUTDOWN,
} pbsys_hmi_ev3_ui_action_t;

#if PBSYS_CONFIG_HMI_EV3_UI

void pbsys_hmi_ev3_ui_initialize(void);

void pbsys_hmi_ev3_ui_stop_bluetooth_activity(void);

void pbsys_hmi_ev3_ui_handle_error(pbio_error_t err);

pbsys_hmi_ev3_ui_action_t pbsys_hmi_ev3_ui_handle_button(pbio_button_flags_t button, uint8_t *payload);

void pbsys_hmi_ev3_ui_draw(void);

void pbsys_hmi_ev3_ui_run_animation_start(void);

void pbsys_hmi_ev3_ui_run_animation_stop(void);

pbio_error_t pbsys_hmi_ev3_ui_closing_credits(pbio_os_state_t *state, void *context);

#else // PBSYS_CONFIG_HMI_EV3_UI

static inline void pbsys_hmi_ev3_ui_initialize(void) {
}

static inline void pbsys_hmi_ev3_ui_stop_bluetooth_activity(void) {
}

static inline void pbsys_hmi_ev3_ui_handle_error(pbio_error_t err) {
}

static inline pbsys_hmi_ev3_ui_action_t pbsys_hmi_ev3_ui_handle_button(pbio_button_flags_t button, uint8_t *payload) {
    return PBSYS_HMI_EV3_UI_ACTION_NONE;
}

static inline void pbsys_hmi_ev3_ui_draw(void) {
}

static inline void pbsys_hmi_ev3_ui_run_animation_start(void) {
}

static inline void pbsys_hmi_ev3_ui_run_animation_stop(void) {
}

pbio_error_t pbsys_hmi_ev3_ui_closing_credits(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBSYS_CONFIG_HMI_EV3_UI

#endif // _PBSYS_SYS_HMI_EV3_UI_H_
