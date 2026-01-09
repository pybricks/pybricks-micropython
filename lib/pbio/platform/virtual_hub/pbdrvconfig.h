// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#define PBDRV_CONFIG_BATTERY                                (1)
#define PBDRV_CONFIG_BATTERY_TEST                           (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                           (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE                  (50 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST                      (1)

// Use Bluetooth simulation locally.
#ifndef PBDRV_CONFIG_RUN_ON_CI
#define PBDRV_CONFIG_BLUETOOTH                              (1)
#define PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS              (2)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK                      (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE_SERVER            (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_POSIX                (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_KIND             (LWP3_HUB_KIND_TECHNIC_LARGE)
#endif // PBDRV_CONFIG_RUN_ON_CI

#define PBDRV_CONFIG_BUTTON                                 (1)
#define PBDRV_CONFIG_BUTTON_TEST                            (1)

#define PBDRV_CONFIG_CLOCK                                  (1)
#ifdef PBDRV_CONFIG_RUN_ON_CI
#define PBDRV_CONFIG_CLOCK_TEST                             (1)
#else
#define PBDRV_CONFIG_CLOCK_LINUX                            (1)
#endif

#define PBDRV_CONFIG_COUNTER                                (1)

#define PBDRV_CONFIG_DISPLAY                                (1)
#define PBDRV_CONFIG_DISPLAY_VIRTUAL                        (1)
#define PBDRV_CONFIG_DISPLAY_NUM_COLS                       (178)
#define PBDRV_CONFIG_DISPLAY_NUM_ROWS                       (128)

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

// USB mock driver used on CI.
#define PBDRV_CONFIG_USB                                    (1)
#define PBDRV_CONFIG_USB_SIMULATION                         (1)
#define PBDRV_CONFIG_USB_MAX_PACKET_SIZE                    (64)
#define PBDRV_CONFIG_USB_NUM_BUFFERED_PACKETS               (2)
#define PBDRV_CONFIG_USB_MFG_STR                            u"Pybricks"
#define PBDRV_CONFIG_USB_PROD_STR                           u"Virtual Hub"
