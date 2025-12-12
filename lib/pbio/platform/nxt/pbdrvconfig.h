// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS NXT

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_NXT                        (1)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_NXT                    (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE          (10 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST              (1)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_NXT                     (1)

#define PBDRV_CONFIG_BLUETOOTH                      (0)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_NXT                      (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (3)
#define PBDRV_CONFIG_COUNTER_NXT                    (1)

#define PBDRV_CONFIG_DISPLAY                        (0)
#define PBDRV_CONFIG_DISPLAY_NUM_COLS               (100)
#define PBDRV_CONFIG_DISPLAY_NUM_ROWS               (64)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_NXT                       (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_HAS_ADC                 (0)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                 (7)

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
#define PBDRV_CONFIG_HAS_PORT_VCC_CONTROL           (1)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_NXT                      (1)

#define PBDRV_CONFIG_RPROC                          (1)
#define PBDRV_CONFIG_RPROC_NXT                      (1)

#define PBDRV_CONFIG_SOUND                          (1)
#define PBDRV_CONFIG_SOUND_DEFAULT_VOLUME           100
#define PBDRV_CONFIG_SOUND_SAMPLED                  (1)
#define PBDRV_CONFIG_SOUND_BEEP_SAMPLED             (1)
#define PBDRV_CONFIG_SOUND_NXT                      (1)

#define PBDRV_CONFIG_STACK                          (1)
#define PBDRV_CONFIG_STACK_EMBEDDED                 (1)

#define PBDRV_CONFIG_USB                            (1)
#define PBDRV_CONFIG_USB_NXT                        (1)
#define PBDRV_CONFIG_USB_MAX_PACKET_SIZE            (64)
#define PBDRV_CONFIG_USB_VID                        LEGO_USB_VID
#define PBDRV_CONFIG_USB_PID                        LEGO_USB_PID_NXT
#define PBDRV_CONFIG_USB_MFG_STR                    LEGO_USB_MFG_STR
#define PBDRV_CONFIG_USB_PROD_STR                   LEGO_USB_PROD_STR_NXT " + Pybricks"
