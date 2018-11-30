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

#ifndef _PBIO_PORT_H_
#define _PBIO_PORT_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * \addtogroup Port I/O ports
 * @{
 */

/**
 * I/O port identifier. The meaning and availability of a port is device-specific.
 */
typedef enum {
    PBIO_PORT_SELF = '@', /**< Virtual port for the programmable brick itself */

#if PBDRV_CONFIG_HAS_PORT_A
    PBIO_PORT_A = 'A', /**< I/O port labeled as "A" */
#endif
#if PBDRV_CONFIG_HAS_PORT_B
    PBIO_PORT_B = 'B', /**< I/O port labeled as "B" */
#endif
#if PBDRV_CONFIG_HAS_PORT_C
    PBIO_PORT_C = 'C', /**< I/O port labeled as "C" */
#endif
#if PBDRV_CONFIG_HAS_PORT_D
    PBIO_PORT_D = 'D', /**< I/O port labeled as "D" */
#endif
#if PBDRV_CONFIG_HAS_PORT_1
    PBIO_PORT_1 = '1', /**< I/O port labeled as "1" */
#endif
#if PBDRV_CONFIG_HAS_PORT_2
    PBIO_PORT_2 = '2', /**< I/O port labeled as "2" */
#endif
#if PBDRV_CONFIG_HAS_PORT_3
    PBIO_PORT_3 = '3', /**< I/O port labeled as "3" */
#endif
#if PBDRV_CONFIG_HAS_PORT_4
    PBIO_PORT_4 = '4', /**< I/O port labeled as "4" */
#endif
} pbio_port_t;

/** @}*/

#endif // _PBIO_PORT_H_
