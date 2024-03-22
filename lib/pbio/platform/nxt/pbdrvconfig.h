// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS NXT

#define PBDRV_CONFIG_ADC                            (0)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_NXT                    (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST              (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST_SIZE         (8 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST_SIZE_USER    (512)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_NXT                     (1)

#define PBDRV_CONFIG_BLUETOOTH                      (0)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_NXT                      (1)

#define PBDRV_CONFIG_IOPORT                         (0)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                 (7)

#define PBDRV_CONFIG_LEGODEV                        (1)
#define PBDRV_CONFIG_LEGODEV_NXT                    (1)
#define PBDRV_CONFIG_LEGODEV_NXT_NUM_MOTOR          (3)
#define PBDRV_CONFIG_LEGODEV_NXT_NUM_SENSOR         (4)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV           (3)
#define PBDRV_CONFIG_MOTOR_DRIVER_NXT               (1)

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)
#define PBDRV_CONFIG_HAS_PORT_1                     (1)
#define PBDRV_CONFIG_HAS_PORT_2                     (1)
#define PBDRV_CONFIG_HAS_PORT_3                     (1)
#define PBDRV_CONFIG_HAS_PORT_4                     (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_ID_C

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_NXT                      (1)

#define PBDRV_CONFIG_SOUND                          (1)
#define PBDRV_CONFIG_SOUND_NXT                      (1)
