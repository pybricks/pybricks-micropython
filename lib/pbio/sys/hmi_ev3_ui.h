// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_HMI_EV3_UI_H_
#define _PBSYS_SYS_HMI_EV3_UI_H_

#include <pbio/button.h>

typedef enum {
    /**
     * No action required.
     */
    PBSYS_HMI_EV3_UI_ACTION_NONE,
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

void pbsys_hmi_ev3_ui_initialize(void);

void pbsys_hmi_ev3_ui_handle_error(pbio_error_t err);

pbsys_hmi_ev3_ui_action_t pbsys_hmi_ev3_ui_handle_button(pbio_button_flags_t button, uint8_t *payload);

void pbsys_hmi_ev3_ui_draw(void);

#endif // _PBSYS_SYS_HMI_EV3_UI_H_
