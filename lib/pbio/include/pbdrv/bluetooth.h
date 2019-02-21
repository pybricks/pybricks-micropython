/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * \addtogroup ButtonDriver Button I/O driver
 * @{
 */

#ifndef _PBDRV_BLUETOOTH_H_
#define _PBDRV_BLUETOOTH_H_

#include "pbdrv/config.h"
#include "pbio/error.h"
#include "sys/process.h"

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

/** @cond INTERNAL */

PROCESS_NAME(pbdrv_bluetooth_hci_process);
PROCESS_NAME(pbdrv_bluetooth_spi_process);

/** @endcond */

#else

static inline pbio_error_t pbdrv_bluetooth_tx(uint8_t c) { return PBIO_ERROR_NOT_SUPPORTED; }

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @}*/
