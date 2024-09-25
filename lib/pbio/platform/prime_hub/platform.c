// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2023 The Pybricks Authors

#include <stdbool.h>

#include <btstack_chipset_cc256x.h>
#undef UNUSED
#include <stm32f4xx_hal.h>

#include <pbdrv/clock.h>
#include "pbio/light_matrix.h"

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/block_device/block_device_w25qxx_stm32.h"
#include "../../drv/bluetooth/bluetooth_btstack_control_gpio.h"
#include "../../drv/bluetooth/bluetooth_btstack_uart_block_stm32_hal.h"
#include "../../drv/bluetooth/bluetooth_btstack.h"
#include "../../drv/charger/charger_mp2639a.h"
#include "../../drv/imu/imu_lsm6ds3tr_c_stm32.h"
#include "../../drv/ioport/ioport_pup.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/legodev/legodev_pup.h"
#include "../../drv/motor_driver/motor_driver_hbridge_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/pwm/pwm_tlc5955_stm32.h"
#include "../../drv/resistor_ladder/resistor_ladder.h"
#include "../../drv/sound/sound_stm32_hal_dac.h"
#include "../../drv/uart/uart_stm32f4_ll_irq.h"
#include "../../drv/usb/usb_stm32.h"

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
    COUNTER_PORT_C,
    COUNTER_PORT_D,
    COUNTER_PORT_E,
    COUNTER_PORT_F,
};

enum {
    LED_DEV_0_STATUS,
    LED_DEV_1_BATTERY,
    LED_DEV_2_BLUETOOTH,
    LED_DEV_3_STATUS_TOP,
    LED_DEV_4_STATUS_BOTTOM,
};

enum {
    LED_ARRAY_DEV_0_LIGHT_MATRIX,
};

enum {
    PWM_DEV_0_TIM1,
    PWM_DEV_1_TIM3,
    PWM_DEV_2_TIM4,
    PWM_DEV_3_TIM12,
    PWM_DEV_4_TIM8,
    PWM_DEV_5_TLC5955,
    PWM_DEV_6_TIM5,
};

enum {
    RESISTOR_LADDER_DEV_0,
    RESISTOR_LADDER_DEV_1,
};

enum {
    UART_PORT_A,
    UART_PORT_B,
    UART_PORT_C,
    UART_PORT_D,
    UART_PORT_E,
    UART_PORT_F,
};

// Bluetooth

const pbdrv_bluetooth_btstack_control_gpio_platform_data_t pbdrv_bluetooth_btstack_control_gpio_platform_data = {
    .enable_gpio = {
        .bank = GPIOA,
        .pin = 2,
    },
};

const pbdrv_bluetooth_btstack_uart_block_stm32_platform_data_t pbdrv_bluetooth_btstack_uart_block_stm32_platform_data = {
    .uart = USART2,
    .uart_irq = USART2_IRQn,
    .tx_dma = DMA1_Stream6,
    .tx_dma_ch = DMA_CHANNEL_4,
    .tx_dma_irq = DMA1_Stream6_IRQn,
    .rx_dma = DMA1_Stream7,
    .rx_dma_ch = DMA_CHANNEL_6,
    .rx_dma_irq = DMA1_Stream7_IRQn,
};

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        GPIO_InitTypeDef gpio_init;

        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF7_USART2;

        // CTS/RTS
        gpio_init.Pin = GPIO_PIN_3 | GPIO_PIN_4;
        gpio_init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOD, &gpio_init);

        // TX/RX
        gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6;
        gpio_init.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOD, &gpio_init);
    }
}

void DMA1_Stream6_IRQHandler(void) {
    pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_tx_dma_irq();
}

void DMA1_Stream7_IRQHandler(void) {
    pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_rx_dma_irq();
}

void USART2_IRQHandler(void) {
    pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_uart_irq();
}

const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data = {
    .uart_block_instance = pbdrv_bluetooth_btstack_uart_block_stm32_hal_instance,
    .chipset_instance = btstack_chipset_cc256x_instance,
    .control_instance = pbdrv_bluetooth_btstack_control_gpio_instance,
    .er_key = (const uint8_t *)UID_BASE,
    .ir_key = (const uint8_t *)UID_BASE,
};

// charger

const pbdrv_charger_mp2639a_platform_data_t pbdrv_charger_mp2639a_platform_data = {
    .mode_pwm_id = PWM_DEV_5_TLC5955,
    .mode_pwm_ch = 15,
    .chg_resistor_ladder_id = RESISTOR_LADDER_DEV_0,
    .chg_resistor_ladder_ch = PBDRV_RESISTOR_LADDER_CH_2,
    .ib_adc_ch = 3,
    .iset_pwm_id = PWM_DEV_6_TIM5,
    .iset_pwm_ch = 1,
};

// IMU

const pbdrv_imu_lsm6s3tr_c_stm32_platform_data_t pbdrv_imu_lsm6s3tr_c_stm32_platform_data = {
    .i2c = I2C2,
};

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef gpio_init;

    // IMU
    if (hi2c->Instance == I2C2) {
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

        // SCL
        gpio_init.Pin = GPIO_PIN_10;

        // do a quick bus reset in case IMU chip is in bad state
        gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        for (int i = 0; i < 10; i++) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
            pbdrv_clock_delay_us(1);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
            pbdrv_clock_delay_us(1);
        }

        // then configure for normal use
        gpio_init.Mode = GPIO_MODE_AF_OD;
        gpio_init.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        // SDA
        gpio_init.Pin = GPIO_PIN_3;
        gpio_init.Alternate = GPIO_AF9_I2C2;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        HAL_NVIC_SetPriority(I2C2_ER_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
        HAL_NVIC_SetPriority(I2C2_EV_IRQn, 3, 2);
        HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);

        // INT1
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        gpio_init.Pin = GPIO_PIN_4;
        gpio_init.Mode = GPIO_MODE_IT_RISING;
        gpio_init.Alternate = 0;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        HAL_NVIC_SetPriority(EXTI4_IRQn, 3, 3);
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    }
}

void I2C2_ER_IRQHandler(void) {
    pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_er_irq();
}

void I2C2_EV_IRQHandler(void) {
    pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_ev_irq();
}

void EXTI4_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

// I/O ports

const pbdrv_legodev_pup_ext_platform_data_t pbdrv_legodev_pup_ext_platform_data[PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .ioport_index = 0,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .ioport_index = 1,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .ioport_index = 2,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .ioport_index = 3,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .ioport_index = 4,
    },
    #if PBDRV_CONFIG_LEGODEV_PUP_NUM_EXT_DEV == PBDRV_CONFIG_IOPORT_NUM_DEV
    {
        .port_id = PBIO_PORT_ID_F,
        .ioport_index = 5,
    },
    #endif
};

const pbdrv_ioport_pup_platform_data_t pbdrv_ioport_pup_platform_data = {
    .port_vcc = { .bank = GPIOA, .pin = 14 },
    .ports = {
        {
            .port_id = PBIO_PORT_ID_A,
            .motor_driver_index = 0,
            .uart_driver_index = UART_PORT_A,
            .pins = {
                .gpio1 = { .bank = GPIOD, .pin = 7  },
                .gpio2 = { .bank = GPIOD, .pin = 8  },
                .uart_buf = { .bank = GPIOA, .pin = 10 },
                .uart_tx = { .bank = GPIOE, .pin = 8  },
                .uart_rx = { .bank = GPIOE, .pin = 7  },
                .uart_alt = GPIO_AF8_UART7,
            },
        },
        {
            .port_id = PBIO_PORT_ID_B,
            .motor_driver_index = 1,
            .uart_driver_index = UART_PORT_B,
            .pins = {
                .gpio1 = { .bank = GPIOD, .pin = 9  },
                .gpio2 = { .bank = GPIOD, .pin = 10 },
                .uart_buf = { .bank = GPIOA, .pin = 8  },
                .uart_tx = { .bank = GPIOD, .pin = 1  },
                .uart_rx = { .bank = GPIOD, .pin = 0  },
                .uart_alt = GPIO_AF11_UART4,
            },
        },
        {
            .port_id = PBIO_PORT_ID_C,
            .motor_driver_index = 2,
            .uart_driver_index = UART_PORT_C,
            .pins = {
                .gpio1 = { .bank = GPIOD, .pin = 11 },
                .gpio2 = { .bank = GPIOE, .pin = 4  },
                .uart_buf = { .bank = GPIOE, .pin = 5  },
                .uart_tx = { .bank = GPIOE, .pin = 1  },
                .uart_rx = { .bank = GPIOE, .pin = 0  },
                .uart_alt = GPIO_AF8_UART8,
            },
        },
        {
            .port_id = PBIO_PORT_ID_D,
            .motor_driver_index = 3,
            .uart_driver_index = UART_PORT_D,
            .pins = {
                .gpio1 = { .bank = GPIOC, .pin = 15 },
                .gpio2 = { .bank = GPIOC, .pin = 14 },
                .uart_buf = { .bank = GPIOB, .pin = 2  },
                .uart_tx = { .bank = GPIOC, .pin = 12 },
                .uart_rx = { .bank = GPIOD, .pin = 2  },
                .uart_alt = GPIO_AF8_UART5,
            },
        },
        {
            .port_id = PBIO_PORT_ID_E,
            .motor_driver_index = 4,
            .uart_driver_index = UART_PORT_E,
            .pins = {
                .gpio1 = { .bank = GPIOC, .pin = 13 },
                .gpio2 = { .bank = GPIOE, .pin = 12 },
                .uart_buf = { .bank = GPIOB, .pin = 5  },
                .uart_tx = { .bank = GPIOE, .pin = 3  },
                .uart_rx = { .bank = GPIOE, .pin = 2  },
                .uart_alt = GPIO_AF11_UART10,
            },
        },
        {
            .port_id = PBIO_PORT_ID_F,
            .motor_driver_index = 5,
            .uart_driver_index = UART_PORT_F,
            .pins = {
                .gpio1 = { .bank = GPIOC, .pin = 11 },
                .gpio2 = { .bank = GPIOE, .pin = 6  },
                .uart_buf = { .bank = GPIOC, .pin = 5  },
                .uart_tx = { .bank = GPIOD, .pin = 15 },
                .uart_rx = { .bank = GPIOD, .pin = 14 },
                .uart_alt = GPIO_AF11_UART9,
            },
        },
    },
};

// LED

const pbdrv_led_dual_platform_data_t pbdrv_led_dual_platform_data[PBDRV_CONFIG_LED_DUAL_NUM_DEV] = {
    {
        .id = LED_DEV_0_STATUS,
        .id1 = LED_DEV_3_STATUS_TOP,
        .id2 = LED_DEV_4_STATUS_BOTTOM,
    },
};

static const pbdrv_led_pwm_platform_color_t pbdrv_led_pwm_color = {
    .r_factor = 1000,
    .g_factor = 170,
    .b_factor = 200,
    .r_brightness = 174,
    .g_brightness = 1590,
    .b_brightness = 327,
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_3_STATUS_TOP,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 5,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 4,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 3,
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_4_STATUS_BOTTOM,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 8,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 7,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 6,
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_1_BATTERY,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 2,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 1,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 0,
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_2_BLUETOOTH,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 20,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 19,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 18,
        .scale_factor = 35,
    },
};

const pbdrv_led_array_pwm_platform_data_t pbdrv_led_array_pwm_platform_data[PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV] = {
    {
        .pwm_chs = (const uint8_t[]) {
            38, 36, 41, 46, 33,
            37, 28, 39, 47, 21,
            24, 29, 31, 45, 23,
            26, 27, 32, 34, 22,
            25, 40, 30, 35, 9
        },
        .num_pwm_chs = 25,
        .pwm_id = PWM_DEV_5_TLC5955,
        .id = LED_ARRAY_DEV_0_LIGHT_MATRIX,
    },
};

// Motor driver

const pbdrv_motor_driver_hbridge_pwm_platform_data_t
    pbdrv_motor_driver_hbridge_pwm_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOE,
        .pin1_gpio.pin = 9,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0_TIM1,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOE,
        .pin2_gpio.pin = 11,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0_TIM1,
        .pin2_pwm_ch = 2,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOE,
        .pin1_gpio.pin = 13,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0_TIM1,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOE,
        .pin2_gpio.pin = 14,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0_TIM1,
        .pin2_pwm_ch = 4,
    },
    // Port C
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 6,
        .pin1_alt = GPIO_AF2_TIM4,
        .pin1_pwm_id = PWM_DEV_2_TIM4,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 7,
        .pin2_alt = GPIO_AF2_TIM4,
        .pin2_pwm_id = PWM_DEV_2_TIM4,
        .pin2_pwm_ch = 2,
    },
    // Port D
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 8,
        .pin1_alt = GPIO_AF2_TIM4,
        .pin1_pwm_id = PWM_DEV_2_TIM4,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 9,
        .pin2_alt = GPIO_AF2_TIM4,
        .pin2_pwm_id = PWM_DEV_2_TIM4,
        .pin2_pwm_ch = 4,
    },
    // Port E
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 6,
        .pin1_alt = GPIO_AF2_TIM3,
        .pin1_pwm_id = PWM_DEV_1_TIM3,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 7,
        .pin2_alt = GPIO_AF2_TIM3,
        .pin2_pwm_id = PWM_DEV_1_TIM3,
        .pin2_pwm_ch = 2,
    },
    // Port F
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 8,
        .pin1_alt = GPIO_AF2_TIM3,
        .pin1_pwm_id = PWM_DEV_1_TIM3,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 1,
        .pin2_alt = GPIO_AF2_TIM3,
        .pin2_pwm_id = PWM_DEV_1_TIM3,
        .pin2_pwm_ch = 4,
    },
};

// PWM

static void pwm_dev_0_platform_init(void) {
}

static void pwm_dev_1_platform_init(void) {
}

static void pwm_dev_2_platform_init(void) {
}

static void pwm_dev_3_platform_init(void) {
    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = GPIO_PIN_15;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF9_TIM12;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // channel 2 has constant 50% duty cycle since it acts as a clock
    TIM12->CCR2 = TIM12->ARR / 2;
}

static void pwm_dev_4_platform_init(void) {
    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = GPIO_PIN_9;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    // channel 4 has constant 50% duty cycle since it acts as a clock
    TIM8->CCR4 = TIM8->ARR / 2;
}

static void pwm_dev_6_platform_init(void) {
    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

// NOTE: Official LEGO firmware uses 1.2 kHz PWM for motors. We have changed to
// 12 kHz to reduce the unpleasant noise (similar to the frequency used by the
// official EV3 firmware).

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM1,
        .prescalar = 8, // results in 12 MHz clock
        .period = 1000, // 12MHz divided by 1k makes 12 kHz PWM
        .id = PWM_DEV_0_TIM1,
        // channel 1/2: Port A motor driver; channel 3/4: Port B motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM3,
        .prescalar = 8, // results in 12 MHz clock
        .period = 1000, // 12MHz divided by 1k makes 12 kHz PWM
        .id = PWM_DEV_1_TIM3,
        // channel 1/2: Port E motor driver; channel 3/4: Port F motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM4,
        .prescalar = 8, // results in 12 MHz clock
        .period = 1000, // 12MHz divided by 1k makes 12 kHz PWM
        .id = PWM_DEV_2_TIM4,
        // channel 1/2: Port C motor driver; channel 3/4: Port D motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_3_platform_init,
        .TIMx = TIM12,
        .prescalar = 1, // results in 96 MHz clock
        .period = 10, // 96 MHz divided by 10 makes 9.6 MHz PWM
        .id = PWM_DEV_3_TIM12,
        // channel 2: TLC5955 GSCLK signal
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE,
    },
    {
        .platform_init = pwm_dev_4_platform_init,
        .TIMx = TIM8,
        .prescalar = 1, // results in 96 MHz clock
        .period = 2930, // 96 MHz divided by 2930 makes 32.765 kHz PWM
        .id = PWM_DEV_4_TIM8,
        // channel 4: Bluetooth 32.768 kHz clock
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE,
    },
    {
        .platform_init = pwm_dev_6_platform_init,
        .TIMx = TIM5,
        .prescalar = 10, // results in 9.6 MHz clock
        .period = 100, // 9.6 MHz divided by 100 makes 96 kHz PWM
        .id = PWM_DEV_6_TIM5,
        // channel 1: Battery charger ISET
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
};

const pbdrv_pwm_tlc5955_stm32_platform_data_t
    pbdrv_pwm_tlc5955_stm32_platform_data[PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV] = {
    {
        .spi = SPI1,
        .spi_irq = SPI1_IRQn,
        .rx_dma = DMA2_Stream2,
        .rx_dma_ch = DMA_CHANNEL_3,
        .rx_dma_irq = DMA2_Stream2_IRQn,
        .tx_dma = DMA2_Stream3,
        .tx_dma_ch = DMA_CHANNEL_3,
        .tx_dma_irq = DMA2_Stream3_IRQn,
        .lat_gpio = GPIOA,
        .lat_gpio_pin = GPIO_PIN_15,
        .id = PWM_DEV_5_TLC5955,
    },
};

// Reset

void pbdrv_reset_power_off(void) {
    // setting PA13 low cuts the power
    GPIOA->BSRR = GPIO_BSRR_BR_13;
}

// resistor ladder

// note: even though the resistors of each ladder have the same values, there
// must be some variations in the loads, so different levels are required
const pbdrv_resistor_ladder_platform_data_t pbdrv_resistor_ladder_platform_data[] = {
    [RESISTOR_LADDER_DEV_0] = {
        .level = { 3642, 3142, 2879, 2634, 2449, 2209, 2072, 1800 },
        .adc_ch = 4,
    },
    [RESISTOR_LADDER_DEV_1] = {
        .level = { 3872, 3394, 3009, 2755, 2538, 2327, 2141, 1969 },
        .adc_ch = 5,
    },
};

// Sound

const pbdrv_sound_stm32_hal_dac_platform_data_t pbdrv_sound_stm32_hal_dac_platform_data = {
    .enable_gpio_bank = GPIOC,
    .enable_gpio_pin = GPIO_PIN_10,
    .dac = DAC1,
    .dac_ch = DAC_CHANNEL_1,
    .dac_trigger = DAC_TRIGGER_T6_TRGO,
    .dma = DMA1_Stream5,
    .dma_ch = DMA_CHANNEL_7,
    .dma_irq = DMA1_Stream5_IRQn,
    .tim = TIM6,
    .tim_clock_rate = 48000000, // APB1: 48MHz
};

void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac) {
    GPIO_InitTypeDef gpio_init;

    gpio_init.Pin = GPIO_PIN_4;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

void DMA1_Stream5_IRQHandler(void) {
    pbdrv_sound_stm32_hal_dac_handle_dma_irq();
}

// UART

const pbdrv_uart_stm32f4_ll_irq_platform_data_t
    pbdrv_uart_stm32f4_ll_irq_platform_data[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART] = {
    [UART_PORT_A] = {
        .uart = UART7,
        .irq = UART7_IRQn,
    },
    [UART_PORT_B] = {
        .uart = UART4,
        .irq = UART4_IRQn,
    },
    [UART_PORT_C] = {
        .uart = UART8,
        .irq = UART8_IRQn,
    },
    [UART_PORT_D] = {
        .uart = UART5,
        .irq = UART5_IRQn,
    },
    [UART_PORT_E] = {
        .uart = UART10,
        .irq = UART10_IRQn,
    },
    [UART_PORT_F] = {
        .uart = UART9,
        .irq = UART9_IRQn,
    },
};

// overrides weak function in setup.m
void UART4_IRQHandler(void) {
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_PORT_B);
}

// overrides weak function in setup.m
void UART5_IRQHandler(void) {
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_PORT_D);
}

// overrides weak function in setup.m
void UART7_IRQHandler(void) {
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_PORT_A);
}

// overrides weak function in setup.m
void UART8_IRQHandler(void) {
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_PORT_C);
}

// overrides weak function in setup.m
void UART9_IRQHandler(void) {
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_PORT_F);
}

// overrides weak function in setup.m
void UART10_IRQHandler(void) {
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_PORT_E);
}

// STM32 HAL integration

// bootloader gives us 16MHz clock
uint32_t SystemCoreClock = 16000000;

// copied from system_stm32.c in stm32 port
const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef gpio_init = { };
    ADC_ChannelConfTypeDef adc_ch_config = { };

    // clocks are enabled in SystemInit
    assert_param(__HAL_RCC_TIM2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_DMA2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_ADC1_IS_CLK_ENABLED());


    // PC0, ADC_IBAT

    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_10;
    adc_ch_config.Rank = 1;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC1, ADCVBAT

    gpio_init.Pin = GPIO_PIN_1;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_11;
    adc_ch_config.Rank = 2;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PB0, BAT_NTC

    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_8;
    adc_ch_config.Rank = 3;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PA3, IBUSBCH

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_3;
    adc_ch_config.Rank = 4;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC4, ADC_PC4

    gpio_init.Pin = GPIO_PIN_4;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_14;
    adc_ch_config.Rank = 5;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PA1, BUTTON2/3/4

    gpio_init.Pin = GPIO_PIN_1;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_1;
    adc_ch_config.Rank = 6;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);
}

void DMA2_Stream0_IRQHandler(void) {
    pbdrv_adc_stm32_hal_handle_irq();
}

void DMA2_Stream2_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_rx_dma_irq(0);
}

void DMA2_Stream3_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_tx_dma_irq(0);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // TLC5955 LED driver
        GPIO_InitTypeDef gpio_init;

        gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &gpio_init);
    }
    if (hspi->Instance == SPI2) {
        // External flash
        GPIO_InitTypeDef gpio_init;

        // /CS, active low
        gpio_init.Pin = GPIO_PIN_12;
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(GPIOB, &gpio_init);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

        // SPI2_SCK
        gpio_init.Pin = GPIO_PIN_13;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        // SPI2_MISO | SPI2_MOSI
        gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        HAL_GPIO_Init(GPIOC, &gpio_init);
    }
}

void SPI1_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_spi_irq(0);
}

void SPI2_IRQHandler(void) {
    pbdrv_block_device_w25qxx_stm32_spi_irq();
}

void DMA1_Stream4_IRQHandler(void) {
    pbdrv_block_device_w25qxx_stm32_spi_handle_tx_dma_irq();
}

void DMA1_Stream3_IRQHandler(void) {
    pbdrv_block_device_w25qxx_stm32_spi_handle_rx_dma_irq();
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        pbdrv_block_device_w25qxx_stm32_spi_rx_complete();
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        pbdrv_pwm_tlc5955_stm32_spi_tx_complete();
    } else if (hspi->Instance == SPI2) {
        pbdrv_block_device_w25qxx_stm32_spi_tx_complete();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        pbdrv_block_device_w25qxx_stm32_spi_error();
    }
}

const pbdrv_block_device_w25qxx_stm32_platform_data_t pbdrv_block_device_w25qxx_stm32_platform_data = {
    .tx_dma = DMA1_Stream4,
    .tx_dma_ch = DMA_CHANNEL_0,
    .tx_dma_irq = DMA1_Stream4_IRQn,
    .rx_dma = DMA1_Stream3,
    .rx_dma_ch = DMA_CHANNEL_0,
    .rx_dma_irq = DMA1_Stream3_IRQn,
    .spi = SPI2,
    .irq = SPI2_IRQn,
    .pin_ncs = {
        .bank = GPIOB,
        .pin = 12,
    },
};

// USB

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    GPIO_InitTypeDef gpio_init;

    // Data pins
    gpio_init.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_HIGH;
    gpio_init.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // VBUS pin
    gpio_init.Pin = GPIO_PIN_9;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 1);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    // ensure correct inital state
    HAL_GPIO_EXTI_Callback(GPIO_PIN_9);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd) {
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);

    // The VBUS IRQ remains enabled so that it can still
    // be triggered if the device is shut down but left
    // connected to charge. When the charging cable is
    // disconnected, the IRQ will trigger and lead to the
    // device fully powering down.
}

void OTG_FS_IRQHandler(void) {
    pbdrv_usb_stm32_handle_otg_fs_irq();
}

void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_4) {
        pbdrv_imu_lsm6ds3tr_c_stm32_handle_int1_irq();
    } else if (pin == GPIO_PIN_9) {
        pbdrv_usb_stm32_handle_vbus_irq(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9));
    }
}

// Early initialization

// special memory addresses defined in linker script
extern uint32_t *_fw_isr_vector_src;

// Called from assembly code in startup.s
void SystemInit(void) {
    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // since the firmware starts at 0x08008000, we need to set the vector table offset
    SCB->VTOR = (uint32_t)&_fw_isr_vector_src;

    // Using external 16Mhz oscillator
    RCC_OscInitTypeDef osc_init;
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc_init.HSEState = RCC_HSE_ON;
    osc_init.LSEState = RCC_LSE_OFF;
    osc_init.HSIState = RCC_HSI_OFF;
    osc_init.LSIState = RCC_LSI_OFF;
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc_init.PLL.PLLM = 8; // VCO_IN 2MHz (16MHz / 8)
    osc_init.PLL.PLLN = 96; // VCO_OUT 192MHz (2MHz * 96)
    osc_init.PLL.PLLP = RCC_PLLP_DIV2; // PLLCLK 96MHz (not using max of 100 because of USB)
    osc_init.PLL.PLLQ = 4; // 48MHz USB clock
    osc_init.PLL.PLLR = 2; // 96MHz system clock

    HAL_RCC_OscConfig(&osc_init);

    RCC_ClkInitTypeDef clk_init;
    clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;  // HCLK 96MHz
    clk_init.APB1CLKDivider = RCC_HCLK_DIV2; // 48MHz
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1; // 96MHz

    HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5);

    // enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
        RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_UART4EN | RCC_APB1ENR_UART5EN |
        RCC_APB1ENR_UART7EN | RCC_APB1ENR_UART8EN | RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN |
        RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM5EN | RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM12EN |
        RCC_APB1ENR_I2C2EN | RCC_APB1ENR_DACEN | RCC_APB1ENR_SPI2EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM8EN | RCC_APB2ENR_UART9EN |
        RCC_APB2ENR_UART10EN | RCC_APB2ENR_ADC1EN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

    // Keep main power on (PA13)
    GPIO_InitTypeDef gpio_init = {
        .Pin = GPIO_PIN_13,
        .Mode = GPIO_MODE_OUTPUT_PP,
    };
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_SET);
}
