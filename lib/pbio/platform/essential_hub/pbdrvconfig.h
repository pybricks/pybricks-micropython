// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

#define PBDRV_CONFIG_ADC                            (1)
#define PBDRV_CONFIG_ADC_STM32_HAL                  (1)
#define PBDRV_CONFIG_ADC_STM32_HAL_ADC_INSTANCE     ADC1
#define PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS 5
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_INSTANCE     DMA2_Stream0
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_CHANNEL      DMA_CHANNEL_0
#define PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ          DMA2_Stream0_IRQn
#define PBDRV_CONFIG_ADC_STM32_HAL_TIMER_INSTANCE   TIM8
#define PBDRV_CONFIG_ADC_STM32_HAL_TIMER_TRIGGER    ADC_EXTERNALTRIGCONV_T8_TRGO

#define PBDRV_CONFIG_BATTERY                        (1)
#define PBDRV_CONFIG_BATTERY_ADC                    (1)
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_CH         1
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_VOLTAGE_SCALED_MAX 8768
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CORRECTION 3
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_CH         0
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_OFFSET 0
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_RAW_MAX    4096
#define PBDRV_CONFIG_BATTERY_ADC_CURRENT_SCALED_MAX 2001
#define PBDRV_CONFIG_BATTERY_ADC_TYPE               (2)
#define PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE        (1)
#define PBDRV_CONFIG_BATTERY_ADC_TEMPERATURE_CH     2

#define PBDRV_CONFIG_BLUETOOTH                      (1)
#define PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE         515
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK              (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_CONTROL_GPIO (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_UART   (1)
#define PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_KIND     LWP3_HUB_KIND_TECHNIC_SMALL
#undef PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_VARIANT_ADDR

#define PBDRV_CONFIG_BLOCK_DEVICE                   (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32      (1)
#define PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q32 (1)
// Carve out 256K from the reserved 1M area at the start of the flash.
// This avoids touching the file system, the area read by the LEGO
// bootloader and the area used by upstream MicroPython. Currently, this
// just needs to be big enough to back up the user program on shutdown.
#define PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_START_ADDRESS (512 * 1024)
#define PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SIZE (256 * 1024)

#define PBDRV_CONFIG_BUTTON                         (1)
#define PBDRV_CONFIG_BUTTON_GPIO                    (1)
#define PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON         (1)
#define PBDRV_CONFIG_BUTTON_GPIO_DEBOUNCE           (1)

#define PBDRV_CONFIG_CHARGER                        (1)
#define PBDRV_CONFIG_CHARGER_MP2639A                (1)

#define PBDRV_CONFIG_CLOCK                          (1)
#define PBDRV_CONFIG_CLOCK_STM32                    (1)

#define PBDRV_CONFIG_COUNTER                        (0)

#define PBDRV_CONFIG_GPIO                           (1)
#define PBDRV_CONFIG_GPIO_STM32F4                   (1)

#define PBDRV_CONFIG_IMU                            (1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32           (1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_X    (1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Y    (-1)
#define PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32_SIGN_Z    (-1)

#define PBDRV_CONFIG_IOPORT                         (1)
#define PBDRV_CONFIG_IOPORT_NUM_DEV                 (2)
#define PBDRV_CONFIG_IOPORT_PUP_QUIRK_POWER_CYCLE   (0)

#define PBDRV_CONFIG_LED                            (1)
#define PBDRV_CONFIG_LED_NUM_DEV                    (2)
#define PBDRV_CONFIG_LED_PWM                        (1)
#define PBDRV_CONFIG_LED_PWM_NUM_DEV                (2)

#define PBDRV_CONFIG_MOTOR_DRIVER                   (1)
#define PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV           (2)
#define PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM       (1)

#define PBDRV_CONFIG_PWM                            (1)
#define PBDRV_CONFIG_PWM_NUM_DEV                    (4)
#define PBDRV_CONFIG_PWM_STM32_TIM                  (1)
#define PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV          (3)
#define PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS      (1)
#define PBDRV_CONFIG_PWM_LP50XX_STM32               (1)
#define PBDRV_CONFIG_PWM_LP50XX_STM32_NUM_DEV       (1)

#define PBDRV_CONFIG_RESET                          (1)
#define PBDRV_CONFIG_RESET_STM32                    (1)
#define PBDRV_CONFIG_RESET_STM32_HAS_BLE_BOOTLOADER (0)

#define PBDRV_CONFIG_UART                           (1)
#define PBDRV_CONFIG_UART_STM32F4_LL_IRQ            (1)
#define PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART   (2)

#define PBDRV_CONFIG_USB                            (1)
#define PBDRV_CONFIG_USB_VID                        LEGO_USB_VID
#define PBDRV_CONFIG_USB_PID                        LEGO_USB_PID_SPIKE_ESSENTIAL
#define PBDRV_CONFIG_USB_MFG_STR                    LEGO_USB_MFG_STR
#define PBDRV_CONFIG_USB_PROD_STR                   LEGO_USB_PROD_STR_TECHNIC_SMALL_HUB " + Pybricks"
#define PBDRV_CONFIG_USB_STM32F4                    (1)

#define PBDRV_CONFIG_WATCHDOG                       (1)
#define PBDRV_CONFIG_WATCHDOG_STM32                 (1)

#define PBDRV_CONFIG_HAS_PORT_A (1)
#define PBDRV_CONFIG_HAS_PORT_B (1)

#define PBDRV_CONFIG_SYS_CLOCK_RATE 96000000
#define PBDRV_CONFIG_INIT_ENABLE_INTERRUPTS_ARM (1)
