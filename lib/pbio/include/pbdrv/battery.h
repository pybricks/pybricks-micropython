/**
 * \addtogroup BatteryDriver Battery I/O driver
 * @{
 */

#ifndef _PBDRV_BATTERY_H_
#define _PBDRV_BATTERY_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/port.h>

/** @cond INTERNAL */
void _pbdrv_battery_init(void);
void _pbdrv_battery_poll(uint32_t now);
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_battery_deinit(void);
#else
static inline void _pbdrv_battery_deinit(void) { }
#endif
/** @endcond */

/**
 * Gets the battery voltage.
 * @param [in]  port    The I/O port
 * @param [out] value   The voltage in millivolts
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_PORT if
 *                      the port is not valid or ::PBIO_ERROR_IO if there was
 *                      an I/O error.
 */
pbio_error_t pbdrv_battery_get_voltage_now(pbio_port_t port, uint16_t *value);

/**
 * Gets the battery current.
 * @param [in]  port    The I/O port
 * @param [out] value   The current in milliamps
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_INVALID_PORT if
 *                      the port is not valid or ::PBIO_ERROR_IO if there was
 *                      an I/O error.
 */
pbio_error_t pbdrv_battery_get_current_now(pbio_port_t port, uint16_t *value);

#endif /* _PBDRV_BATTERY_H_ */

/** @} */
