// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include <stdbool.h>

#include <stm32f4xx_hal.h>

#include "pbio/uartdev.h"
#include "pbio/light_matrix.h"

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/bluetooth/bluetooth_btstack_control_gpio.h"
#include "../../drv/bluetooth/bluetooth_btstack_uart_block_stm32_hal.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/pwm/pwm_tlc5955_stm32.h"
#include "../../drv/sound/sound_stm32_hal_dac.h"
#include "../../drv/uart/uart_stm32f4_ll_irq.h"

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
    LED_DEV_1_STATUS_TOP,
    LED_DEV_2_STATUS_BOTTOM,
    LED_DEV_3_BATTERY,
    LED_DEV_4_BLUETOOTH,
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

// I/O ports

const pbdrv_ioport_lpf2_platform_data_t pbdrv_ioport_lpf2_platform_data = {
    .port_vcc = { .bank = GPIOA, .pin = 14 },
    .ports = {
        // Port A
        {
            .id1 = { .bank = GPIOD, .pin = 7  },
            .id2 = { .bank = GPIOD, .pin = 8  },
            .uart_buf = { .bank = GPIOA, .pin = 10 },
            .uart_tx = { .bank = GPIOE, .pin = 8  },
            .uart_rx = { .bank = GPIOE, .pin = 7  },
            .alt = GPIO_AF8_UART7,
        },
        // Port B
        {
            .id1 = { .bank = GPIOD, .pin = 9  },
            .id2 = { .bank = GPIOD, .pin = 10 },
            .uart_buf = { .bank = GPIOA, .pin = 8  },
            .uart_tx = { .bank = GPIOD, .pin = 1  },
            .uart_rx = { .bank = GPIOD, .pin = 0  },
            .alt = GPIO_AF11_UART4,
        },
        // Port C
        {
            .id1 = { .bank = GPIOD, .pin = 11 },
            .id2 = { .bank = GPIOE, .pin = 4  },
            .uart_buf = { .bank = GPIOE, .pin = 5  },
            .uart_tx = { .bank = GPIOE, .pin = 1  },
            .uart_rx = { .bank = GPIOE, .pin = 0  },
            .alt = GPIO_AF8_UART8,
        },
        // Port D
        {
            .id1 = { .bank = GPIOC, .pin = 15 },
            .id2 = { .bank = GPIOC, .pin = 14 },
            .uart_buf = { .bank = GPIOB, .pin = 2  },
            .uart_tx = { .bank = GPIOC, .pin = 12 },
            .uart_rx = { .bank = GPIOD, .pin = 2  },
            .alt = GPIO_AF8_UART5,
        },
        // Port E
        {
            .id1 = { .bank = GPIOC, .pin = 13 },
            .id2 = { .bank = GPIOE, .pin = 12 },
            .uart_buf = { .bank = GPIOB, .pin = 5  },
            .uart_tx = { .bank = GPIOE, .pin = 3  },
            .uart_rx = { .bank = GPIOE, .pin = 2  },
            .alt = GPIO_AF11_UART10,
        },
        // Port F
        {
            .id1 = { .bank = GPIOC, .pin = 11 },
            .id2 = { .bank = GPIOE, .pin = 6  },
            .uart_buf = { .bank = GPIOC, .pin = 5  },
            .uart_tx = { .bank = GPIOD, .pin = 15 },
            .uart_rx = { .bank = GPIOD, .pin = 14 },
            .alt = GPIO_AF11_UART9,
        },
    },
};

// LED

const pbdrv_led_dual_platform_data_t pbdrv_led_dual_platform_data[PBDRV_CONFIG_LED_DUAL_NUM_DEV] = {
    {
        .id = LED_DEV_0_STATUS,
        .id1 = LED_DEV_1_STATUS_TOP,
        .id2 = LED_DEV_2_STATUS_BOTTOM,
    },
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .id = LED_DEV_1_STATUS_TOP,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 5,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 4,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 3,
        .scale_factor = 35,
    },
    {
        .id = LED_DEV_2_STATUS_BOTTOM,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 8,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 7,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 6,
        .scale_factor = 35,
    },
    {
        .id = LED_DEV_3_BATTERY,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 2,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 1,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 0,
        .scale_factor = 35,
    },
    {
        .id = LED_DEV_4_BLUETOOTH,
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

void pbdrv_reset_stm32_power_off(void) {
    // setting PA13 low cuts the power
    GPIOA->BSRR = GPIO_BSRR_BR_13;
}

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

const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [COUNTER_PORT_A] = {
        .uart_id = UART_PORT_A,
        .counter_id = COUNTER_PORT_A,
    },
    [COUNTER_PORT_B] = {
        .uart_id = UART_PORT_B,
        .counter_id = COUNTER_PORT_B,
    },
    [COUNTER_PORT_C] = {
        .uart_id = UART_PORT_C,
        .counter_id = COUNTER_PORT_C,
    },
    [COUNTER_PORT_D] = {
        .uart_id = UART_PORT_D,
        .counter_id = COUNTER_PORT_D,
    },
    [COUNTER_PORT_E] = {
        .uart_id = UART_PORT_E,
        .counter_id = COUNTER_PORT_E,
    },
    [COUNTER_PORT_F] = {
        .uart_id = UART_PORT_F,
        .counter_id = COUNTER_PORT_F,
    },
};


// STM32 HAL integration

// bootloader gives us 16MHz clock
uint32_t SystemCoreClock = 16000000;

// copied from system_stm32.c in stm32 port
const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef gpio_init = { 0 };
    ADC_ChannelConfTypeDef adc_ch_config = { 0 };

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
}

void SPI1_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_spi_irq(0);
}

// USB

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    GPIO_InitTypeDef GPIO_InitStruct;

    // Data pins
    GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // VBUS pin
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd) {
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
}

void OTG_FS_IRQHandler(void) {
    extern PCD_HandleTypeDef hpcd;
    HAL_PCD_IRQHandler(&hpcd);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef gpio_init;

    // IMU
    if (hi2c->Instance == I2C2) {
        gpio_init.Mode = GPIO_MODE_AF_OD;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

        // SCL
        gpio_init.Pin = GPIO_PIN_10;
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
    }
}

void I2C2_ER_IRQHandler(void) {
    extern void mod_experimental_IMU_handle_i2c_er_irq(void);
    mod_experimental_IMU_handle_i2c_er_irq();
}

void I2C2_EV_IRQHandler(void) {
    extern void mod_experimental_IMU_handle_i2c_ev_irq(void);
    mod_experimental_IMU_handle_i2c_ev_irq();
}

// Early initialization

// optional handling for dual-boot firmware
__WEAK void pbio_platform_dual_boot(void) {
}

// special memory addresses defined in linker script
extern uint32_t *_fw_isr_vector_src;

// Called from assembly code in startup.s
void SystemInit(void) {
    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // since the firmware starts at 0x08008000, we need to set the vector table offset
    SCB->VTOR = (uint32_t)&_fw_isr_vector_src;

    // bootloader disables interrupts
    __enable_irq();

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

    // If we are running dual boot, jump to other firmware if no buttons are pressed
    pbio_platform_dual_boot();

    // enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
        RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_UART4EN | RCC_APB1ENR_UART5EN |
        RCC_APB1ENR_UART7EN | RCC_APB1ENR_UART8EN | RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN |
        RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM12EN | RCC_APB1ENR_I2C2EN |
        RCC_APB1ENR_DACEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM8EN | RCC_APB2ENR_UART9EN |
        RCC_APB2ENR_UART10EN | RCC_APB2ENR_ADC1EN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

    // Keep main power on (PA13)
    GPIOA->BSRR = GPIO_BSRR_BS_13;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER13_Msk) | (1 << GPIO_MODER_MODER13_Pos);
}
