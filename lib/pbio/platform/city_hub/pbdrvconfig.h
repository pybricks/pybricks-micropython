// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

// platform-specific configuration for LEGO Powered Up Smart Hub

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_STM32F0                    (1)

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC                    (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         11
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX    3893
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX 9600
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         10
// FIXME: these values come from LEGO firmware, but seem to be 2x current
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX    4095
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX 2444

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (1)

#define PBDRV_CONFIG_BLUETOOTH                      (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (2)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32F0                   (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_LPF2                    (1)
#define PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS          (2)
#define PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT         PBIO_PORT_A
#define PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT          PBIO_PORT_B

#define PBDRV_CONFIG_LIGHT                          (1)

#define PBDRV_CONFIG_MOTOR                          (1)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (3)
#define PBDRV_CONFIG_PWM_STM32_TIM                  (1)
#define PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV          (3)
#define PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS      (0)

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_STM32F0                   (1)
#define PBDRV_CONFIG_UART_STM32F0_NUM_UART          (2)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)

#define PBDRV_CONFIG_FIRST_IO_PORT          PBIO_PORT_A
#define PBDRV_CONFIG_LAST_IO_PORT           PBIO_PORT_B
#define PBDRV_CONFIG_NUM_IO_PORT            (2)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_B
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (2)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 48000000

#endif // _PBDRVCONFIG_H_
