/*
 * Copyright (c) 2018 David Lechner
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
