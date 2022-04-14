// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS NXT

#define PBDRV_CONFIG_ADC                            (0)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_NXT                    (1)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_NXT                     (1)

#define PBDRV_CONFIG_BLUETOOTH                      (0)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_NXT                      (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NXT                    (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (3)
#define PBDRV_CONFIG_COUNTER_NXT_NUM_DEV            (3)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_NXT                     (1)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV           (3)
#define PBDRV_CONFIG_MOTOR_DRIVER_NXT               (1)

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_ID_C
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (3)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_NXT                      (1)
