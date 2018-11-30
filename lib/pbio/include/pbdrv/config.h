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

#ifndef _PBDRV_CONFIG_H_
#define _PBDRV_CONFIG_H_

// This file is defined per-platform. It should override the config values below as needed.
#include "pbdrvconfig.h"

// set to (1) if there is a port labeled "A" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_A
#define PBDRV_CONFIG_HAS_PORT_A (0)
#endif

// set to (1) if there is a port labeled "B" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_B
#define PBDRV_CONFIG_HAS_PORT_B (0)
#endif

// set to (1) if there is a port labeled "C" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_C
#define PBDRV_CONFIG_HAS_PORT_C (0)
#endif

// set to (1) if there is a port labeled "D" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_D
#define PBDRV_CONFIG_HAS_PORT_D (0)
#endif

// set to (1) if there is a port labeled "1" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_1
#define PBDRV_CONFIG_HAS_PORT_1 (0)
#endif

// set to (1) if there is a port labeled "2" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_2
#define PBDRV_CONFIG_HAS_PORT_2 (0)
#endif

// set to (1) if there is a port labeled "3" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_3
#define PBDRV_CONFIG_HAS_PORT_3 (0)
#endif

// set to (1) if there is a port labeled "4" on the programmable brick
#ifndef PBDRV_CONFIG_HAS_PORT_4
#define PBDRV_CONFIG_HAS_PORT_4 (0)
#endif

// the number of I/O ports in the programmable brick
#ifndef PBDRV_CONFIG_NUM_IO_PORT
#define PBDRV_CONFIG_NUM_IO_PORT (0)
#elif PBDRV_CONFIG_NUM_IO_PORT != 0

// the pbio_port_t enum value of the first I/O port
#ifndef PBDRV_CONFIG_FIRST_IO_PORT
#error PBDRV_CONFIG_NUM_IO_PORT requires that PBDRV_CONFIG_FIRST_IO_PORT is defined
#endif

// the pbio_port_t enum value of the last I/O port
#ifndef PBDRV_CONFIG_LAST_IO_PORT
#error PBDRV_CONFIG_NUM_IO_PORT requires that PBDRV_CONFIG_LAST_IO_PORT is defined
#endif
#endif

// the number of built-in motor controllers in the programmable brick
#ifndef PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER (0)
#elif PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

// the pbio_port_t enum value of the first motor port
#ifndef PBDRV_CONFIG_FIRST_MOTOR_PORT
#error PBDRV_CONFIG_NUM_MOTOR_CONTROLLER requires that PBDRV_CONFIG_FIRST_MOTOR_PORT is defined
#endif

// the pbio_port_t enum value of the last motor port
#ifndef PBDRV_CONFIG_LAST_MOTOR_PORT
#error PBDRV_CONFIG_NUM_MOTOR_CONTROLLER requires that PBDRV_CONFIG_LAST_MOTOR_PORT is defined
#endif
#endif

// enable the A/DC driver
#ifndef PBDRV_CONFIG_ADC
#define PBDRV_CONFIG_ADC (1)
#endif

// enable the battery driver
#ifndef PBDRV_CONFIG_BATTERY
#define PBDRV_CONFIG_BATTERY (1)
#endif

// enable the bluetooth driver
#ifndef PBDRV_CONFIG_BLUETOOTH
#define PBDRV_CONFIG_BLUETOOTH (1)
#endif

// enable the button driver
#ifndef PBDRV_CONFIG_BUTTON
#define PBDRV_CONFIG_BUTTON (1)
#endif

// enable the I/O port driver
#ifndef PBDRV_CONFIG_IOPORT
#define PBDRV_CONFIG_IOPORT (1)
#endif

// enable the light driver
#ifndef PBDRV_CONFIG_LIGHT
#define PBDRV_CONFIG_LIGHT (1)
#endif

// enable the motor driver
#ifndef PBDRV_CONFIG_MOTOR
#define PBDRV_CONFIG_MOTOR (1)
#endif

// enable the UART driver
#ifndef PBDRV_CONFIG_UART
#define PBDRV_CONFIG_UART (1)
#endif

#endif // _PBDRV_CONFIG_H_
