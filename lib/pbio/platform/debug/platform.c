// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>
#include <pbio/uartdev.h>

#include "stm32f4xx_hal.h"

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/button/button_gpio.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/uart/uart_stm32f4_ll_irq.h"


enum {
    LED_DEV_0,
};

enum {
    PWM_DEV_0,
    PWM_DEV_1,
    PWM_DEV_2,
};

enum {
    UART_ID_0,
};

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
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

void DMA2_Stream0_IRQHandler(void) {
    pbdrv_adc_stm32_hal_handle_irq();
}

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = { .bank = GPIOC, .pin = 13 },
        .button = PBIO_BUTTON_CENTER,
    }
};

// LED

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .id = LED_DEV_0,
        .r_id = PWM_DEV_2,
        .r_ch = 1,
        .g_id = PWM_DEV_0,
        .g_ch = 3,
        .b_id = PWM_DEV_1,
        .b_ch = 2,
        .scale_factor = 5,
    }
};

// PWM

static void pwm_dev_0_platform_init(void) {
    // green LED LD1 on PB0 using TIM3 CH3
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER0_Msk) | (2 << GPIO_MODER_MODER0_Pos);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL0_Msk) | (2 << GPIO_AFRL_AFSEL0_Pos);
}

static void pwm_dev_1_platform_init(void) {
    // blue LED LD2 on PB7 using TIM4 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL7_Msk) | (2 << GPIO_AFRL_AFSEL7_Pos);
}

static void pwm_dev_2_platform_init(void) {
    // red LED LD3 on PB14 using TIM12 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (9 << GPIO_AFRH_AFSEL14_Pos);
}

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM3,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_0,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM4,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_1,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM12,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_2,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
};

// UART Config
//
// Currently using pins labeled USART on the Nucleo board as a UART sensor port.
// UART sensors can be wired to the port with 22Ω series resistors on Tx and Rx.

const pbdrv_uart_stm32f4_ll_irq_platform_data_t
    pbdrv_uart_stm32f4_ll_irq_platform_data[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART] = {
    [UART_ID_0] = {
        .uart = USART2,
        .irq = USART2_IRQn,
    },
};

const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id = UART_ID_0,
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
    pbdrv_uart_stm32f4_ll_irq_handle_irq(UART_ID_0);
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

    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));  /* set CP10 and CP11 Full Access */
    #endif

    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // Using internal 16Mhz oscillator
    RCC_OscInitTypeDef osc_init;
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

    RCC_ClkInitTypeDef clk_init;
    clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;  // HCLK 48MHz (max 180MHz)
    clk_init.APB1CLKDivider = RCC_HCLK_DIV1; // 48MHz (max 45MHz)
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1; // 48MHz (max 90MHz)

    HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5);

    // enable all of the hardware modules we are using
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
        RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
        RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN |
        RCC_APB1ENR_TIM12EN | RCC_APB1ENR_USART2EN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC3EN | RCC_APB2ENR_USART6EN;

    // UART for terminal
    GPIOG->MODER = (GPIOG->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
    GPIOG->AFR[1] = (GPIOG->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) | (8 << GPIO_AFRH_AFSEL9_Pos);
    GPIOG->MODER = (GPIOG->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOG->AFR[1] = (GPIOG->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (8 << GPIO_AFRH_AFSEL14_Pos);
    USART6->BRR = (26 << 4) | 1; // 48MHz/(16*26.0625) = 115107 baud
    USART6->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void assert_failed(uint8_t *file, uint32_t line) {
    // set a breakpoint here for debugging
}
