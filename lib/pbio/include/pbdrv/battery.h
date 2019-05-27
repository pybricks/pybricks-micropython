// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

/**
 * \addtogroup BatteryDriver Battery I/O driver
 * @{
 */

#ifndef _PBDRV_BATTERY_H_
#define _PBDRV_BATTERY_H_

#include <pbdrv/config.h>

#include <pbio/error.h>

#if PBDRV_CONFIG_BATTERY

#include <stdint.h>

#include <sys/process.h>

/** @cond INTERNAL */
PROCESS_NAME(pbdrv_battery_process);
/** @endcond */

/**
 * Gets the battery voltage.
 * @param [out] value   The voltage in millivolts
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_PORT if
 *                      the port is not valid or ::PBIO_ERROR_IO if there was
 *                      an I/O error.
 */
pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value);

/**
 * Gets the battery current.
 * @param [out] value   The current in milliamps
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_PORT if
 *                      the port is not valid or ::PBIO_ERROR_IO if there was
 *                      an I/O error.
 */
pbio_error_t pbdrv_battery_get_current_now(uint16_t *value);

#else

static inline void _pbdrv_battery_poll(uint32_t now) { }
static inline void _pbdrv_battery_deinit(void) { }
static inline pbio_error_t pbdrv_battery_get_voltage_now(uint16_t *value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_battery_get_current_now(uint16_t *value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif /* _PBDRV_BATTERY_H_ */

/** @} */
