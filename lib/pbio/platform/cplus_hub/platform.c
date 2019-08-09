// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <stdbool.h>

#include "pbio/uartdev.h"

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/button/button_gpio.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/uart/uart_stm32_hal.h"

#include "stm32l4xx_hal.h"
#include "stm32l4xx_ll_rcc.h"

// PBIO driver data

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio   = { .bank = GPIOC, .pin = 14 },
        .pull   = PBDRV_GPIO_PULL_NONE,
        .button = PBIO_BUTTON_CENTER,
        .active_low = true,
    },
};

// Port A
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_0 = {
    .id1        = { .bank = GPIOC, .pin = 5  },
    .id2        = { .bank = GPIOB, .pin = 2  },
    .uart_buf   = { .bank = GPIOH, .pin = 0  },
    .uart_tx    = { .bank = GPIOB, .pin = 6  },
    .uart_rx    = { .bank = GPIOB, .pin = 7  },
    .alt        = GPIO_AF7_USART1,
};

// Port B
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_1 = {
    .id1        = { .bank = GPIOC, .pin = 3  },
    .id2        = { .bank = GPIOC, .pin = 0  },
    .uart_buf   = { .bank = GPIOH, .pin = 1  },
    .uart_tx    = { .bank = GPIOA, .pin = 2  },
    .uart_rx    = { .bank = GPIOA, .pin = 3  },
    .alt        = GPIO_AF7_USART2,
};

// Port C
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_2 = {
    .id1        = { .bank = GPIOC, .pin = 4  },
    .id2        = { .bank = GPIOA, .pin = 7  },
    .uart_buf   = { .bank = GPIOC, .pin = 8  },
    .uart_tx    = { .bank = GPIOC, .pin = 10 },
    .uart_rx    = { .bank = GPIOC, .pin = 11 },
    .alt        = GPIO_AF7_USART3,
};

// Port D
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_3 = {
    .id1        = { .bank = GPIOA, .pin = 4  },
    .id2        = { .bank = GPIOA, .pin = 5  },
    .uart_buf   = { .bank = GPIOC, .pin = 7  },
    .uart_tx    = { .bank = GPIOB, .pin = 11 },
    .uart_rx    = { .bank = GPIOB, .pin = 10  },
    .alt        = GPIO_AF8_LPUART1,
};

enum {
    UART_PORT_A,
    UART_PORT_B,
    UART_PORT_C,
    UART_PORT_D,
};

const pbdrv_uart_stm32_hal_platform_data_t
pbdrv_uart_stm32_hal_platform_data[PBDRV_CONFIG_UART_STM32_HAL_NUM_UART] = {
    [UART_PORT_A] = {
        .uart   = USART1,
        .irq    = USART1_IRQn,
    },
    [UART_PORT_B] = {
        .uart   = USART2,
        .irq    = USART2_IRQn,
    },
    [UART_PORT_C] = {
        .uart   = USART3,
        .irq    = USART3_IRQn,
    },
    // [UART_PORT_D] = {
    //     .uart   = LPUART1,
    //     .irq    = LPUART1_IRQn,
    // },
};

void USART1_IRQHandler() {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_A);
}

void USART2_IRQHandler() {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_B);
}

void USART3_IRQHandler() {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_C);
}

// void LPUART1_IRQHandler() {
//     pbdrv_uart_stm32_hal_handle_irq(UART_PORT_D);
// }

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
    COUNTER_PORT_C,
    COUNTER_PORT_D,
};

const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id    = UART_PORT_A,
        .counter_id = COUNTER_PORT_A,
    },
    [1] = {
        .uart_id    = UART_PORT_B,
        .counter_id = COUNTER_PORT_B,
    },
    [2] = {
        .uart_id    = UART_PORT_C,
        .counter_id = COUNTER_PORT_C,
    },
    // [3] = {
    //     .uart_id    = UART_PORT_D,
    //     .counter_id = COUNTER_PORT_D,
    // },
};


// STM32 HAL integration

// Default clock is 4MHz on boot
uint32_t SystemCoreClock = 4000000;

// copied from system_stm32.c in stm32 port
const uint8_t  AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
const uint8_t  APBPrescTable[8] =  { 0, 0, 0, 0, 1, 2, 3, 4 };
const uint32_t MSIRangeTable[12] = {
    100000, 200000, 400000, 800000, 1000000, 2000000,
    4000000, 8000000, 16000000, 24000000, 32000000, 48000000
};

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
    GPIO_InitTypeDef gpio_init = { 0 };
    ADC_ChannelConfTypeDef adc_ch_config = { 0 };

    // clocks are enabled in SystemInit
    assert_param(__HAL_RCC_TIM2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_DMA1_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_ADC1_IS_CLK_ENABLED());

    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_SYSCLK);

    // PC1, battery voltage

    gpio_init.Pin = GPIO_PIN_1;
    gpio_init.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_2;
    adc_ch_config.Rank = ADC_REGULAR_RANK_1;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC2, battery current

    gpio_init.Pin = GPIO_PIN_2;
    gpio_init.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_3;
    adc_ch_config.Rank = ADC_REGULAR_RANK_2;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // internal, temperature

    adc_ch_config.Channel = ADC_CHANNEL_TEMPSENSOR;
    adc_ch_config.Rank = ADC_REGULAR_RANK_3;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);
}

void DMA1_Channel1_IRQHandler() {
    pbdrv_adc_stm32_hal_handle_irq();
}


// Early initialization

// special memory addresses defined in linker script
extern uint32_t *_fw_isr_vector_src;

TIM_HandleTypeDef *cplus_hub_htim1;
TIM_HandleTypeDef *cplus_hub_htim15;
TIM_HandleTypeDef *cplus_hub_htim16;

// Called from assembly code in startup.s
void SystemInit(void) {
    RCC_OscInitTypeDef osc_init = { 0 };
    RCC_ClkInitTypeDef clk_init = { 0 };
    GPIO_InitTypeDef gpio_init = { 0 };
    static UART_HandleTypeDef huart;
    static TIM_HandleTypeDef htim1;
    static TIM_HandleTypeDef htim15;
    static TIM_HandleTypeDef htim16;

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

    // Using external 16Mhz oscillator
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    osc_init.MSIState = RCC_MSI_ON;
    osc_init.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc_init.MSIClockRange = RCC_MSIRANGE_6; // 4MHz
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    osc_init.PLL.PLLM = 1; // VCO_IN 4MHz (4MHz / 1)
    osc_init.PLL.PLLN = 40; // VCO_OUT 160MHz (4MHz * 40)
    osc_init.PLL.PLLP = RCC_PLLP_DIV7; // 22.9MHz SAI clock
    osc_init.PLL.PLLQ = RCC_PLLQ_DIV4; // 40MHz SDMMC1, RNG and USB clocks
    osc_init.PLL.PLLR = RCC_PLLR_DIV2; // 80MHz system clock

    HAL_RCC_OscConfig(&osc_init);

    clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider  = RCC_SYSCLK_DIV1; // HCLK 80MHz
    clk_init.APB1CLKDivider = RCC_HCLK_DIV1; // disassembly shows DIV16, but 80MHz is OK according to data sheet
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_4);

    // dpgeorge: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN | RCC_AHB1ENR_FLASHEN |
                    RCC_AHB1ENR_CRCEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN |
                    RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOHEN | RCC_AHB2ENR_ADCEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN | RCC_APB1ENR1_WWDGEN | RCC_APB1ENR1_USART2EN |
                     RCC_APB1ENR1_USART3EN | RCC_APB1ENR1_I2C1EN | RCC_APB1ENR1_PWREN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SPI1EN |
                    RCC_APB2ENR_USART1EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN;


    // Keep main power on (PC12)
    gpio_init.Pin = GPIO_PIN_12;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOC, &gpio_init);

    // Turn VCC_PORT on (PB12)
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOB, &gpio_init);

    // FIXME: temporarily using port D for terminal until we get Bluetooth working
    gpio_init.Pin = GPIO_PIN_7;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
    HAL_GPIO_Init(GPIOC, &gpio_init);
    gpio_init.Pin = GPIO_PIN_11;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Alternate = GPIO_AF8_LPUART1;
    HAL_GPIO_Init(GPIOB, &gpio_init);
    gpio_init.Pin = GPIO_PIN_10;
    HAL_GPIO_Init(GPIOB, &gpio_init);
    huart.Instance = LPUART1;
    huart.Init.BaudRate = 115200;
    HAL_UART_Init(&huart);
    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_RE;

    // shared timers

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 8 - 1;
    htim1.Init.Period = 10000 -1;
    HAL_TIM_Base_Init(&htim1);

    htim15.Instance = TIM15;
    htim15.Init.Prescaler = 8 - 1;
    htim15.Init.Period = 10000 -1;
    HAL_TIM_Base_Init(&htim15);

    htim16.Instance = TIM16;
    htim16.Init.Prescaler = 8 - 1;
    htim16.Init.Period = 10000 -1;
    HAL_TIM_Base_Init(&htim16);

    cplus_hub_htim1 = &htim1;
    cplus_hub_htim15 = &htim15;
    cplus_hub_htim16 = &htim16;

    // since the firmware starts at 0x08008000, we need to set the vector table offset
    SCB->VTOR = (uint32_t)&_fw_isr_vector_src;

    // bootloader disables interrupts?
    __enable_irq();
}
