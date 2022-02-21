// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup ResetDriver Driver: System Power and Reset
 * @{
 */

#ifndef _PBDRV_RESET_H_
#define _PBDRV_RESET_H_

#include <pbdrv/config.h>

/** Reset/power actions. */
typedef enum {
    /** Resets the MCU without powering off. */
    PBDRV_RESET_ACTION_RESET = 0,
    /** Turns off power to the hub/brick. */
    PBDRV_RESET_ACTION_POWER_OFF = 1,
    /** Resets the MCU in firmware update mode. */
    PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE = 2,
} pbdrv_reset_action_t;

/** Reasons for reset. */
typedef enum {
    /** No reset occurred. Normal power on. */
    PBDRV_RESET_REASON_NONE = 0,
    /** Reset was triggered by software. */
    PBDRV_RESET_REASON_SOFTWARE = 1,
    /** Reset was triggered by watchdog timer. */
    PBDRV_RESET_REASON_WATCHDOG = 2,
} pbdrv_reset_reason_t;

#if PBDRV_CONFIG_RESET

/**
 * Resets or powers off the hub/brick. This function does not return.
 * @param [in]  action  The action to perform.
 */
void pbdrv_reset(pbdrv_reset_action_t action) __attribute__((noreturn));

/**
 * Switches the power off.
 *
 * The device may not actually power down if the button is pressed or USB is
 * plugged in.
 *
 * Unlike pbdrv_reset(PBDRV_RESET_ACTION_POWER_OFF), this function will return.
 */
void pbdrv_reset_power_off(void);

/**
 * Gets the reason for the most recent reset.
 * @return The reason.
 */
pbdrv_reset_reason_t pbdrv_reset_get_reason(void);

#else // PBDRV_CONFIG_RESET

static inline void pbdrv_reset(pbdrv_reset_action_t action) {
}

static inline void pbdrv_reset_power_off(void) {
}

static inline pbdrv_reset_reason_t pbdrv_reset_get_reason(void) {
    return PBDRV_RESET_REASON_NONE;
}

#endif // PBDRV_CONFIG_RESET

#endif // _PBDRV_RESET_H_

/** @} */
