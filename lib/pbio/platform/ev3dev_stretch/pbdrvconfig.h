// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

// platform-specific configuration for LEGO MINDSTORMS EV3 running ev3dev-stretch

#define PBDRV_CONFIG_HUB_EV3BRICK

#define PBDRV_CONFIG_BATTERY                                (1)
#define PBDRV_CONFIG_BATTERY_LINUX_EV3                      (1)

#define PBDRV_CONFIG_COUNTER                                (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                        (4)
#define PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO             (1)
#define PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV     (4)

#define PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE              (2)

#define PBDRV_CONFIG_IOPORT                                 (1)
#define PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH                  (1)

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

#define PBDRV_CONFIG_BLUETOOTH  (0)
#define PBDRV_CONFIG_BUTTON     (1)
#define PBDRV_CONFIG_UART       (0)

#endif // _PBDRVCONFIG_H_
