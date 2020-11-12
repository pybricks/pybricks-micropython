// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup BatteryDriver Driver: Battery
 * @{
 */

#ifndef _PBDRV_BATTERY_H_
#define _PBDRV_BATTERY_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/** Battery chemistry types. */
typedef enum {
    /** The battery type is not known. */
    PBDRV_BATTERY_TYPE_UNKNOWN,
    /** The batteries are alkaline (e.g. AA/AAA). */
    PBDRV_BATTERY_TYPE_ALKALINE,
    /** The batteries are Li-ion. */
    PBDRV_BATTERY_TYPE_LIION,
} pbdrv_battery_type_t;

#if PBDRV_CONFIG_BATTERY

/** @cond INTERNAL */
void pbdrv_battery_init();
/** @endcond */

/**
 * Gets the battery voltage.
 * @param [out] value   The voltage in millivolts.
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_NOT_SUPPORTED if
 *                      this function is not support on the current platform
 *                      or ::PBIO_ERROR_IO if there was an I/O error.
 */
pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value);

/**
 * Gets the battery current.
 * @param [out] value   The current in milliamps.
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_NOT_SUPPORTED if
 *                      this function is not support on the current platform
 *                      or ::PBIO_ERROR_IO if there was an I/O error.
 */
pbio_error_t pbdrv_battery_get_current_now(uint16_t *value);

/**
 * Gets the battery chemistry type.
 * @param [out] value   The type of battery.
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_NOT_SUPPORTED if
 *                      this function is not support on the current platform
 *                      or ::PBIO_ERROR_IO if there was an I/O error.
 */
pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value);

/**
 * Gets the battery temperature.
 * @param [out] value   The temperature in millidegrees Celsius.
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_NOT_SUPPORTED if
 *                      this function is not support on the current platform
 *                      or ::PBIO_ERROR_IO if there was an I/O error.
 */
pbio_error_t pbdrv_battery_get_temperature(uint32_t *value);

#else // PBDRV_CONFIG_BATTERY

#define pbdrv_battery_init()

static inline pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_battery_get_type(pbdrv_battery_type_t *value) {
    *value = PBDRV_BATTERY_TYPE_UNKNOWN;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_battery_get_temperature(uint32_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BATTERY

#endif // _PBDRV_BATTERY_H_

/** @} */
