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
