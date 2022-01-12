// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

/**
 * @addtogroup ChargerDriver Driver: Battery Charger
 * @{
 */

#ifndef _PBDRV_CHARGER_H_
#define _PBDRV_CHARGER_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/** Battery charger status indication. */
typedef enum {
    /** The battery is being discharged. */
    PBDRV_CHARGER_STATUS_DISCHARGE,
    /** The battery is being charged. */
    PBDRV_CHARGER_STATUS_CHARGE,
    /** Charging is complete/the battery is full. */
    PBDRV_CHARGER_STATUS_COMPLETE,
    /** The charger has detected a problem. */
    PBDRV_CHARGER_STATUS_FAULT,
} pbdrv_charger_status_t;

/** Battery charger current limit selection. */
typedef enum {
    /**
     * The charger should be limited to 0 mA.
     *
     * This can be used when the charger is not charging.
     */
    PBDRV_CHARGER_LIMIT_NONE,
    /**
     * The charger should be limited to 100 mA.
     *
     * This is the standard for USB 2.0 ports.
     */
    PBDRV_CHARGER_LIMIT_STD_MIN,
    /**
     * The charger should be limited to 500 mA.
     *
     * This is the standard for USB 2.0 ports after negotiation.
     */
    PBDRV_CHARGER_LIMIT_STD_MAX,
    /**
     * The charger should be limited to 1.5A.
     *
     * This is the standard for both downstream charging ports and dedicated
     * charging ports.
     */
    PBDRV_CHARGER_LIMIT_CHARGING,
} pbdrv_charger_limit_t;

#if PBDRV_CONFIG_CHARGER

/**
 * Gets the current being applied to the battery as measured by the charger.
 * @param [out] current     The current in mV.
 * @return                  ::PBIO_SUCCESS or ::PBIO_ERROR_NOT_SUPPORTED if
 *                          the charger driver is not enabled.
 */
pbio_error_t pbdrv_charger_get_current_now(uint16_t *current);

/**
 * Gets the status of the charger.
 * @return                 The status.
 */
pbdrv_charger_status_t pbdrv_charger_get_status(void);

/**
 * Enables or disables charging.
 * @param [in]  enable  True to enable charging or false for discharging.
 * @param [in]  limit   The current limit for the charging rate.
 */
void pbdrv_charger_enable(bool enable, pbdrv_charger_limit_t limit);

#else

static inline pbio_error_t pbdrv_charger_get_current_now(uint16_t *current) {
    *current = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbdrv_charger_status_t pbdrv_charger_get_status(void) {
    return PBDRV_CHARGER_STATUS_FAULT;
}

static inline void pbdrv_charger_enable(bool enable, pbdrv_charger_limit_t limit) {
}

#endif

#endif // _PBDRV_CHARGER_H_

/** @} */
