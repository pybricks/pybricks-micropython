// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020,2022 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS EV3 running ev3dev-stretch

#define PBDRV_CONFIG_BATTERY                                (1)
#define PBDRV_CONFIG_BATTERY_LINUX_EV3                      (1)

#define PBDRV_CONFIG_BUTTON                                 (1)
#define PBDRV_CONFIG_BUTTON_LINUX_EV3                       (1)

#define PBDRV_CONFIG_CLOCK                                  (1)
#define PBDRV_CONFIG_CLOCK_LINUX                            (1)

#define PBDRV_CONFIG_COUNTER                                (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                        (4)
#define PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO             (1)
#define PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO_NUM_DEV     (4)

#define PBDRV_CONFIG_IOPORT                                 (0)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                         (8)

#define PBDRV_CONFIG_LEGODEV                                (1)
#define PBDRV_CONFIG_LEGODEV_MODE_INFO                      (1)
#define PBDRV_CONFIG_LEGODEV_EV3DEV                         (1)
#define PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_MOTOR               (4)
#define PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_SENSOR              (4)

#define PBDRV_CONFIG_MOTOR_DRIVER                           (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV                   (4)
#define PBDRV_CONFIG_MOTOR_DRIVER_EV3DEV_STRETCH            (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)
#define PBDRV_CONFIG_HAS_PORT_1 (1)
#define PBDRV_CONFIG_HAS_PORT_2 (1)
#define PBDRV_CONFIG_HAS_PORT_3 (1)
#define PBDRV_CONFIG_HAS_PORT_4 (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT PBIO_PORT_ID_D
