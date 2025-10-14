// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#define PBDRV_CONFIG_BATTERY                                (1)
#define PBDRV_CONFIG_BATTERY_TEST                           (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                           (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE                  (10 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST                      (1)

#define PBDRV_CONFIG_BLUETOOTH                              (1)
#define PBDRV_CONFIG_BLUETOOTH_SIMULATION                   (1)

#define PBDRV_CONFIG_BUTTON                                 (1)
#define PBDRV_CONFIG_BUTTON_TEST                            (1)

#define PBDRV_CONFIG_CLOCK                                  (1)
#ifdef PBDRV_CONFIG_CLOCK_LINUX
#define PBDRV_CONFIG_CLOCK_LINUX_SIGNAL                     (1)
#else
#define PBDRV_CONFIG_CLOCK_TEST                             (1)
#endif

#define PBDRV_CONFIG_COUNTER                                (1)

#define PBDRV_CONFIG_GPIO                                   (1)
#define PBDRV_CONFIG_GPIO_VIRTUAL                           (1)

#define PBDRV_CONFIG_IOPORT                                 (1)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                         (6)

#define PBDRV_CONFIG_MOTOR_DRIVER                           (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV                   (6)
#define PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION        (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)
#define PBDRV_CONFIG_HAS_PORT_E (1)
#define PBDRV_CONFIG_HAS_PORT_F (1)
#define PBDRV_CONFIG_HAS_PORT_VCC_CONTROL                   (1)
