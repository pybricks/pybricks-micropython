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
 * \addtogroup BatteryDriver Battery I/O driver
 * @{
 */

#ifndef _PBDRV_BATTERY_H_
#define _PBDRV_BATTERY_H_

#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#if PBDRV_CONFIG_BATTERY

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

#else

static inline void _pbdrv_battery_init(void) { }
static void _pbdrv_battery_poll(uint32_t now) { }
static inline void _pbdrv_battery_deinit(void) { }
static inline pbio_error_t pbdrv_battery_get_voltage_now(pbio_port_t port, uint16_t *value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_battery_get_current_now(pbio_port_t port, uint16_t *value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif /* _PBDRV_BATTERY_H_ */

/** @} */
