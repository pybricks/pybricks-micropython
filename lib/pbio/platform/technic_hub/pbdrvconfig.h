// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_STM32_HAL                  (1)
#define PBDRV_CONFIG_ADC_STM32_HAL_ADC_INSTANCE     ADC1
#define PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS 3
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_INSTANCE     DMA1_Channel1
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_REQUEST      DMA_REQUEST_0
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ          DMA1_Channel1_IRQn
#define PBDRV_CONFIG_ADC_STM32_HAL_TIMER_INSTANCE   TIM6
#define PBDRV_CONFIG_ADC_STM32_HAL_TIMER_TRIGGER    ADC_EXTERNALTRIG_T6_TRGO

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC                    (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         0
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX 9618
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION 12
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         1
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET 20
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX 4178
#define PBDRV_CONFIG_BATTERY_ADC_TYPE               (3)

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32       (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_FLASH_STM32_SIZE  (16 * 1024) // Must match FLASH_USER_0 + FLASH_USER_1 in linker script

#define PBDRV_CONFIG_BLUETOOTH                      (1)
#define PBDRV_CONFIG_BLUETOOTH_STM32_CC2640         (1)
#define PBDRV_CONFIG_BLUETOOTH_STM32_CC2640_HUB_ID  "\x80"

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (1)
#define PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE           (1)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_STM32                    (1)

#define PBDRV_CONFIG_COUNTER                        (1)
#define PBDRV_CONFIG_COUNTER_NUM_DEV                (4)
#define PBDRV_CONFIG_COUNTER_LPF2                   (1)
#define PBDRV_CONFIG_COUNTER_LPF2_NUM_DEV           (4)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32L4                   (1)

#define PBDRV_CONFIG_IMU                            (1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32           (1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_X    (-1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Y    (-1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Z    (1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_LPF2                    (1)
#define PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS          (4)
#define PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT         PBIO_PORT_ID_A
#define PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT          PBIO_PORT_ID_D

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (1)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (1)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV           (4)
#define PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM       (1)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (4)
#define PBDRV_CONFIG_PWM_STM32_TIM                  (1)
#define PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV          (4)
#define PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS      (1)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_STM32                    (1)
#define PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER (1)

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_STM32L4_LL_DMA            (1)
#define PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART   (4)

#define PBDRV_CONFIG_WATCHDOG                       (1)
#define PBDRV_CONFIG_WATCHDOG_STM32                 (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)
#define PBDRV_CONFIG_HAS_PORT_C (1)
#define PBDRV_CONFIG_HAS_PORT_D (1)

#define PBDRV_CONFIG_FIRST_MOTOR_PORT       PBIO_PORT_ID_A
#define PBDRV_CONFIG_LAST_MOTOR_PORT        PBIO_PORT_ID_D
#define PBDRV_CONFIG_NUM_MOTOR_CONTROLLER   (4)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 80000000
