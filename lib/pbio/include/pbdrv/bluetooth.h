// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * \addtogroup ButtonDriver Button I/O driver
 * @{
 */

#ifndef _PBDRV_BLUETOOTH_H_
#define _PBDRV_BLUETOOTH_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

#if PBDRV_CONFIG_BLUETOOTH

/**
 * Queues a character to be transmitted via Bluetooth serial port.
 * @param c [in]    the character to be sent.
 * @return          ::PBIO_SUCCESS if *c* was queued, ::PBIO_ERROR_AGAIN if the
 *                  character could not be queued at this time (e.g. buffer is
 *                  full), ::PBIO_ERROR_INVALID_OP if there is not an active
 *                  Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED if this
 *                  platform does not support Bluetooth.
 */
pbio_error_t pbdrv_bluetooth_tx(uint8_t c);

#else // PBDRV_CONFIG_BLUETOOTH

static inline pbio_error_t pbdrv_bluetooth_tx(uint8_t c) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @}*/
