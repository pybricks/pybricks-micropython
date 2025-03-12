// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

// platform-specific configuration for LEGO MINDSTORMS EV3

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_EV3                        (1)
#define PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS       (16)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_TIAM1808                 (1)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_EV3                    (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST              (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST_SIZE         (8 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_TEST_SIZE_USER    (512)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                 (8)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (6)

#define PBDRV_CONFIG_DISPLAY                        (1)
#define PBDRV_CONFIG_DISPLAY_EV3                    (1)
#define PBDRV_CONFIG_DISPLAY_NUM_COLS               (178)
#define PBDRV_CONFIG_DISPLAY_NUM_ROWS               (128)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_TIAM1808                  (1)

#define PBDRV_CONFIG_HAS_PORT_A                     (1)
#define PBDRV_CONFIG_HAS_PORT_B                     (1)
#define PBDRV_CONFIG_HAS_PORT_C                     (1)
#define PBDRV_CONFIG_HAS_PORT_D                     (1)
#define PBDRV_CONFIG_HAS_PORT_1                     (1)
#define PBDRV_CONFIG_HAS_PORT_2                     (1)
#define PBDRV_CONFIG_HAS_PORT_3                     (1)
#define PBDRV_CONFIG_HAS_PORT_4                     (1)

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (3)
#define PBDRV_CONFIG_LED_DUAL                       (1)
#define PBDRV_CONFIG_LED_DUAL_NUM_DEV               (1)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (2)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (2)
#define PBDRV_CONFIG_PWM_TIAM1808                   (1)
#define PBDRV_CONFIG_PWM_TIAM1808_NUM_DEV           (2)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_EV3                      (1)

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_DEBUG_FIRST_PORT          (1)
#define PBDRV_CONFIG_UART_EV3                       (1)
#define PBDRV_CONFIG_UART_EV3_PRU                   (1)
#define PBDRV_CONFIG_UART_EV3_NUM_UART              (5)
