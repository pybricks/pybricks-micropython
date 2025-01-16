// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// platform-specific configuration for LEGO Powered Up Smart Hub

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_STM32F0                    (1)
#define PBDRV_CONFIG_ADC_STM32F0_RANDOM             (1)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC                    (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         11
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX 10100
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION 12
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         10
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET 130
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX 2448
#define PBDRV_CONFIG_BATTERY_ADC_TYPE               (1)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (1)
#define PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE           (1)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32       (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE  (16 * 1024) // Must match FLASH_USER_0 + FLASH_USER_1 in linker script

#define PBDRV_CONFIG_BLUETOOTH                      (1)
#define PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE         158 // 158 matches LEGO firmware - could go up to ~251 - see ATT_MAX_MTU_SIZE
#define PBDRV_CONFIG_BLUETOOTH_STM32_CC2640         (1)
#define PBDRV_CONFIG_BLUETOOTH_STM32_CC2640_HUB_ID  "\x41"

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_STM32                    (1)

#define PBDRV_CONFIG_COUNTER                        (0)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32F0                   (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_PUP                     (1)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                 (2)
#define PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE   (1)

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (1)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (1)

#define PBDRV_CONFIG_LEGODEV                        (1)
#define PBDRV_CONFIG_LEGODEV_PUP                    (1)
#define PBDRV_CONFIG_LEGODEV_PUP_NUM_INT_DEV        (0)
#define PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV        (2)
#define PBDRV_CONFIG_LEGODEV_PUP_UART               (1)
#define PBDRV_CONFIG_LEGODEV_MODE_INFO              (1)
#define PBDRV_CONFIG_LEGODEV_PUP_UART_NUM_DEV       (PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV           (2)
#define PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM       (1)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (3)
#define PBDRV_CONFIG_PWM_STM32_TIM                  (1)
#define PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV          (3)
#define PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS      (0)

#define PBDRV_CONFIG_RANDOM                         (1)
#define PBDRV_CONFIG_RANDOM_ADC                     (1)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_STM32                    (1)
#define PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER (1)

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_STM32F0                   (1)
#define PBDRV_CONFIG_UART_STM32F0_NUM_UART          (2)

#define PBDRV_CONFIG_WATCHDOG                       (1)
#define PBDRV_CONFIG_WATCHDOG_STM32                 (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_ID_B

#define PBDRV_CONFIG_SYS_CLOCK_RATE 48000000
#define PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM     (1)
