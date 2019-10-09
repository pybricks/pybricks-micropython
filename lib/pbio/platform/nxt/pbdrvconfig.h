// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

// platform-specific configuration for LEGO MINDSTORMS NXT

#define PBDRV_CONFIG_ADC                            (0)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_NXT                    (1)

#define PBDRV_CONFIG_LIGHT                          (0)

#define PBDRV_CONFIG_BUTTON                         (1)

#define PBDRV_CONFIG_BLUETOOTH                      (0)

#define PBDRV_CONFIG_COUNTER                        (0)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (2)

#define PBDRV_CONFIG_UART                           (0)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)

// #define PBDRV_CONFIG_FIRST_IO_PORT          PBIO_PORT_A
// #define PBDRV_CONFIG_LAST_IO_PORT           PBIO_PORT_B

// #define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_A
// #define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_C
// #define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (3)

#endif // _PBDRVCONFIG_H_
