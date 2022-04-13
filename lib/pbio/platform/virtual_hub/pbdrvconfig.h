// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#define PBDRV_CONFIG_BATTERY                                (1)
#define PBDRV_CONFIG_BATTERY_VIRTUAL                        (1)

#define PBDRV_CONFIG_BUTTON                                 (1)
#define PBDRV_CONFIG_BUTTON_VIRTUAL                         (1)

#define PBDRV_CONFIG_CLOCK                                  (1)
#define PBDRV_CONFIG_CLOCK_LINUX                            (1)
#define PBDRV_CONFIG_CLOCK_LINUX_SIGNAL                     (1)

#define PBDRV_CONFIG_COUNTER                                (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                        (6)
#define PBDRV_CONFIG_COUNTER_VIRTUAL                        (1)
#define PBDRV_CONFIG_COUNTER_VIRTUAL_NUM_DEV                (6)

#define PBDRV_CONFIG_LED                                    (1)
#define PBDRV_CONFIG_LED_NUM_DEV                            (1)
#define PBDRV_CONFIG_LED_VIRTUAL                            (1)
#define PBDRV_CONFIG_LED_VIRTUAL_NUM_DEV                    (1)

#define PBDRV_CONFIG_MOTOR_DRIVER                           (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV                   (6)
#define PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL                   (1)

#define PBDRV_CONFIG_VIRTUAL                                (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)
#define PBDRV_CONFIG_HAS_PORT_E (1)
#define PBDRV_CONFIG_HAS_PORT_F (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_ID_F
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (6)
