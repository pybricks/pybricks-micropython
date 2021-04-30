// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

/**
 * @addtogroup ResetDriver Driver: System Power and Reset
 * @{
 */

#ifndef _PBDRV_RESET_H_
#define _PBDRV_RESET_H_

/** Reset/power actions. */
typedef enum {
    /** Resets the MCU without powering off. */
    PBDRV_RESET_ACTION_RESET,
    /** Turns off power to the hub/brick. */
    PBDRV_RESET_ACTION_POWER_OFF,
    /** Resets the MCU in firmware update mode. */
    PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE,
} pbdrv_reset_action_t;

/** Reasons for reset. */
typedef enum {
    /** No reset occurred. Normal power on. */
    PBDRV_RESET_REASON_NONE,
    /** Reset was triggered by software. */
    PBDRV_RESET_REASON_SOFTWARE,
    /** Reset was triggered by watchdog timer. */
    PBDRV_RESET_REASON_WATCHDOG,
} pbdrv_reset_reason_t;

/**
 * Resets or powers off the hub/brick. This function does not return.
 * @param [in]  action  The action to perform.
 */
void pbdrv_reset(pbdrv_reset_action_t action) __attribute__((noreturn));

/**
 * Gets the reason for the most recent reset.
 * @return The reason.
 */
pbdrv_reset_reason_t pbdrv_reset_get_reason(void);

#endif // _PBDRV_RESET_H_

/** @} */
