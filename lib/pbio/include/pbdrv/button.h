// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

/**
 * \addtogroup ButtonDriver Button I/O driver
 * @{
 */

#ifndef _PBDRV_BUTTON_H_
#define _PBDRV_BUTTON_H_

#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

#if PBDRV_CONFIG_BUTTON

/** @cond INTERNAL */

/**
 * Initializes the low level button driver. This should be called only
 * once and must be called before using any other button functions.
 */
void _pbdrv_button_init(void);


/**
 * Releases the low level button driver. No button functions can be called after
 * calling this function.
 */
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void);
#else
static inline void _pbdrv_button_deinit(void) { }
#endif

/** @endcond */

/**
 * Get bitmask indicating currently pressed buttons.
 * @param [in] port         The port to read
 * @param [out] pressed     Bitmask indicating which buttons are pressed
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_NO_DEV if port is valid but a device with buttons is not connected
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed);

#else

static inline void _pbdrv_button_init(void) { }
static inline void _pbdrv_button_deinit(void) { }
static inline pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif // _PBDRV_BUTTON_H_

/** @}*/
