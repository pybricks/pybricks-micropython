// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBDRVCONFIG_H_
#define _PBDRVCONFIG_H_

#define PBDRV_CONFIG_HUB_PRIMEHUB

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_STM32_HAL                  (1)
#define PBDRV_CONFIG_ADC_STM32_HAL_ADC_INSTANCE     ADC1
#define PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS 6
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_INSTANCE     DMA2_Stream0
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_CHANNEL      DMA_CHANNEL_0
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ          DMA2_Stream0_IRQn
#define PBDRV_CONFIG_ADC_STM32_HAL_TIMER_INSTANCE   TIM2
#define PBDRV_CONFIG_ADC_STM32_HAL_TIMER_TRIGGER    ADC_EXTERNALTRIGCONV_T2_TRGO

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC                    (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         1
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX 9900
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION 3
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         0
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET 0
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX 7300
#define PBDRV_CONFIG_BATTERY_ADC_TYPE               (2)
#define PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE        (1)
#define PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE_CH     2

#define PBDRV_CONFIG_BLUETOOTH                      (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK              (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_CC264X (1)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_ADC                     (1)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_STM32                    (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (6)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32F4                   (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_LPF2                    (1)
#define PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS          (6)
#define PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT         PBIO_PORT_A
#define PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT          PBIO_PORT_F

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (5)
#define PBDRV_CONFIG_LED_DUAL                       (1)
#define PBDRV_CONFIG_LED_DUAL_NUM_DEV               (1)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (4)

#define PBDRV_CONFIG_LED_ARRAY                      (1)
#define PBDRV_CONFIG_LED_ARRAY_NUM_DEV              (1)
#define PBDRV_CONFIG_LED_ARRAY_PWM                  (1)
#define PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV          (1)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (6)
#define PBDRV_CONFIG_PWM_STM32_TIM                  (1)
#define PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV          (5)
#define PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS      (1)
#define PBDRV_CONFIG_PWM_TLC5955_STM32              (1)
#define PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV      (1)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_STM32                    (1)
#define PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER (0)

#define PBDRV_CONFIG_SOUND                          (1)
#define PBDRV_CONFIG_SOUND_STM32_HAL_DAC            (1)

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_STM32F4_LL_IRQ            (1)
#define PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART   (6)

#define PBDRV_CONFIG_USB                            (1)
#define PBDRV_CONFIG_USB_STM32F4_CDC                (1)

#define PBDRV_CONFIG_MOTOR          (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)
#define PBDRV_CONFIG_HAS_PORT_E (1)
#define PBDRV_CONFIG_HAS_PORT_F (1)

#define PBDRV_CONFIG_FIRST_IO_PORT          PBIO_PORT_A
#define PBDRV_CONFIG_LAST_IO_PORT           PBIO_PORT_F
#define PBDRV_CONFIG_NUM_IO_PORT            (6)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_F
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (6)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 96000000

#endif // _PBDRVCONFIG_H_
