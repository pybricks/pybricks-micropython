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
