// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup ResetDriver System Power and Reset Driver
 * @{
 */

#ifndef _PBDRV_RESET_H_
#define _PBDRV_RESET_H_

/** Reset/power actions. */
typedef enum {
    /** Turns off power to the hub/brick. */
    PBDRV_RESET_ACTION_POWER_OFF,
    /** Resets the MCU without powering off. */
    PBDRV_RESET_ACTION_RESET,
    /** Resets the MCU in firmware update mode. */
    PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE,
} pbdrv_reset_action_t;

/**
 * Resets or powers off the hub/brick. This function does not return.
 * @param [in]  action  The action to perform.
 */
void pbdrv_reset(pbdrv_reset_action_t action) __attribute__((noreturn));

#endif // _PBDRV_RESET_H_

/**
 * @}
 */
