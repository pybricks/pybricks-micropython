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

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

// platform-specific configuration for LEGO MINDSTORMS EV3 running ev3dev-stretch

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)
#define PBDRV_CONFIG_HAS_PORT_1 (1)
#define PBDRV_CONFIG_HAS_PORT_2 (1)
#define PBDRV_CONFIG_HAS_PORT_3 (1)
#define PBDRV_CONFIG_HAS_PORT_4 (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT PBIO_PORT_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT PBIO_PORT_D
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER (4)

#define PBDRV_CONFIG_ADC        (0)
#define PBDRV_CONFIG_BLUETOOTH  (0)
#define PBDRV_CONFIG_BUTTON     (1)
#define PBDRV_CONFIG_IOPORT     (0)
#define PBDRV_CONFIG_UART       (0)

#endif // _PBDRVCONFIG_H_
