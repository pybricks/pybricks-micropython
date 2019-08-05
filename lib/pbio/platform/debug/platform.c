// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>
#include <pbio/uartdev.h>

#define USE_HAL_DRIVER
#include "stm32f4xx.h"

#include "../../drv/button/button_gpio.h"
#include "../../drv/uart/uart_stm32_hal.h"

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
    GPIO_InitTypeDef gpio_init;
    ADC_ChannelConfTypeDef adc_ch_config;

    // clocks are enabled in SystemInit
    assert_param(__HAL_RCC_TIM2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_DMA2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_ADC3_IS_CLK_ENABLED());

    // PA3, A0

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_3;
    adc_ch_config.Rank = 1;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC0, A1

    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_10;
    adc_ch_config.Rank = 2;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC3, A2

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_13;
    adc_ch_config.Rank = 3;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PF3, A3

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_9;
    adc_ch_config.Rank = 4;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PF5, A4

    gpio_init.Pin = GPIO_PIN_5;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_15;
    adc_ch_config.Rank = 5;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PF10, A5

    gpio_init.Pin = GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_8;
    adc_ch_config.Rank = 6;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);
}

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio   = { .bank = GPIOC, .pin = 13 },
        .button = PBIO_BUTTON_CENTER,
    }
};

// UART Config
//
// Currently using pins labeled USART on the Nucleo board as a UART sensor port.
// UART sensors can be wired to the port with 22Ω series resistors on Tx and Rx.

enum {
    UART_ID_0,
};

const pbdrv_uart_stm32_hal_platform_data_t
pbdrv_uart_stm32_hal_platform_data[PBDRV_CONFIG_UART_STM32_HAL_NUM_UART] = {
    [UART_ID_0] = {
        .uart   = USART2,
        .irq    = USART2_IRQn,
    },
};

const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id    = UART_ID_0,
    },
};

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef gpio_init;

    // clocks are enabled in sys.c

    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = 7;

    gpio_init.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOD, &gpio_init);

    gpio_init.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOD, &gpio_init);
}

// overrides weak function in setup.m
void USART2_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_ID_0);
}

// HACK: we don't have a generic ioport interface yet so defining this function
// in platform.c
pbio_error_t pbdrv_ioport_get_iodev(pbio_port_t port, pbio_iodev_t **iodev) {
    if (port != PBIO_PORT_1) {
        return PBIO_ERROR_INVALID_PORT;
    }

    return pbio_uartdev_get(0, iodev);
}

uint32_t SystemCoreClock = 16000000;

// copied from system_stm32.c in stm32 port
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void) {
    RCC_OscInitTypeDef osc_init;
    RCC_ClkInitTypeDef clk_init;

    // Using internal 16Mhz oscillator
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc_init.HSEState = RCC_HSE_ON;
    osc_init.HSIState = RCC_HSI_OFF;
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc_init.PLL.PLLM = 4; // VCO_IN 2MHz (8MHz / 4)
    osc_init.PLL.PLLN = 96; // VCO_OUT 192MHz (2MHz * 168)
    osc_init.PLL.PLLP = RCC_PLLP_DIV4; // PLLCLK 48MHz (to match move hub/city hub)
    osc_init.PLL.PLLQ = 4; // 48MHz USB clock (192MHz / 4)

    HAL_RCC_OscConfig(&osc_init);

    clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1; // HCLK 48MHz (max 180MHz)
    clk_init.APB1CLKDivider = RCC_HCLK_DIV1; // 48MHz (max 45MHz)
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1; // 48MHz (max 90MHz)

    HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5);

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

    // dpgeorge: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // enable GPIO clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
                    RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_USART2EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN | RCC_APB2ENR_USART6EN;

    // UART for terminal
    GPIOG->MODER = (GPIOG->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
    GPIOG->AFR[1] = (GPIOG->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) | (8 << GPIO_AFRH_AFSEL9_Pos);
    GPIOG->MODER = (GPIOG->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOG->AFR[1] = (GPIOG->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (8 << GPIO_AFRH_AFSEL14_Pos);
    USART6->BRR = (26 << 4) | 1; // 48MHz/(16*26.0625) = 115107 baud
    USART6->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void assert_failed(uint8_t* file, uint32_t line) {
    // set a breakpoint here for debugging
}
