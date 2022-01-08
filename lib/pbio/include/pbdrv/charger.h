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

typedef enum {
    PBDRV_CHARGER_STATUS_CHARGE,
    PBDRV_CHARGER_STATUS_DISCHARGE,
    PBDRV_CHARGER_STATUS_FAULT,
} pbdrv_charger_status_t;

/**
 * Indicates the type of the connected USB port.
 */
typedef enum {
    /** The USB cable is not connected (no VBUS) */
    PBDRV_CHARGER_USB_TYPE_NONE,
    /** The USB cable is connected to a non-standard charger or PS/2 port. */
    PBDRV_CHARGER_USB_TYPE_NONSTANDARD,
    /** The USB cable is connected to standard downstream port. */
    PBDRV_CHARGER_USB_TYPE_STANDARD_DOWNSTREAM,
    /** The USB cable is connected to charging downstream port. */
    PBDRV_CHARGER_USB_TYPE_CHARGING_DOWNSTREAM,
    /** The USB cable is connected to dedicated charging port. */
    PBDRV_CHARGER_USB_TYPE_DEDICATED_CHARGING,
} pbdrv_charger_usb_type_t;

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
 * Gets the USB charger type.
 * @return              The type.
 */
pbdrv_charger_usb_type_t pbdrv_charger_get_usb_type(void);

/**
 * Enables or disables charging.
 * @param [in]  enable  True to enable charging or false for discharging.
 */
void pbdrv_charger_enable(bool enable);

#else

static inline pbio_error_t pbdrv_charger_get_current_now(uint16_t *current) {
    *current = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbdrv_charger_status_t pbdrv_charger_get_status(void) {
    return PBDRV_CHARGER_STATUS_FAULT;
}

static inline pbdrv_charger_usb_type_t pbdrv_charger_get_usb_type(void) {
    return PBDRV_CHARGER_USB_TYPE_NONE;
}

static inline void pbdrv_charger_enable(bool enable) {
}

#endif

#endif // _PBDRV_CHARGER_H_

/** @} */
