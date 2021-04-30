// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

// platform-specific configuration for LEGO BOOST Move Hub

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_STM32F0                    (1)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC                    (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         11
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX 10100
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION 12
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         10
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET 0
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX 2448
#define PBDRV_CONFIG_BATTERY_ADC_TYPE               (1)

#define PBDRV_CONFIG_BLUETOOTH                      (1)
#define PBDRV_CONFIG_BLUETOOTH_STM32_BLUENRG        (1)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (1)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_STM32                    (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (4)
#define PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC  (1)
#define PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC_NUM_DEV (2)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32F0                   (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_LPF2                    (1)
#define PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS          (2)
#define PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT         PBIO_PORT_C
#define PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT          PBIO_PORT_D

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (1)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (1)

#define PBDRV_CONFIG_MOTOR                          (1)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (4)
#define PBDRV_CONFIG_PWM_STM32_TIM                  (1)
#define PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV          (4)
#define PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS      (0)

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
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_D
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (4)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 48000000

#endif // _PBDRVCONFIG_H_
