// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <stdbool.h>

#include "pbio/uartdev.h"

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/uart/uart_stm32_hal.h"

#include "stm32f4xx_hal.h"

// bootlaoder magic

typedef struct {
    const char *fw_ver;
    const uint32_t *checksum;
    const uint8_t *magic;
    const void *reserved;
} boot_t;

// defined in linker script
extern const uint32_t _checksum;

// special boot info table read by bootloader
const boot_t __attribute__((section(".boot"))) boot = {
    .fw_ver = "v0.5.00.0020-2b83556",
    .checksum = &_checksum,
    .magic = (const uint8_t[16]) {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4},
};


// Port A - UART7
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_0 = {
    .id1 = { .bank = GPIOD, .pin = 7  },
    .id2 = { .bank = GPIOD, .pin = 8  },
    .uart_buf = { .bank = GPIOA, .pin = 10 },
    .uart_tx = { .bank = GPIOE, .pin = 8  },
    .uart_rx = { .bank = GPIOE, .pin = 7  },
    .alt = 8,
};

// Port B - UART4
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_1 = {
    .id1 = { .bank = GPIOD, .pin = 9  },
    .id2 = { .bank = GPIOD, .pin = 10 },
    .uart_buf = { .bank = GPIOA, .pin = 8  },
    .uart_tx = { .bank = GPIOD, .pin = 1  },
    .uart_rx = { .bank = GPIOD, .pin = 0  },
    .alt = 11,
};

// Port C - UART8
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_2 = {
    .id1 = { .bank = GPIOD, .pin = 11 },
    .id2 = { .bank = GPIOE, .pin = 4  },
    .uart_buf = { .bank = GPIOE, .pin = 5  },
    .uart_tx = { .bank = GPIOE, .pin = 1  },
    .uart_rx = { .bank = GPIOE, .pin = 0  },
    .alt = 8,
};

// Port D - UART5
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_3 = {
    .id1 = { .bank = GPIOC, .pin = 15 },
    .id2 = { .bank = GPIOC, .pin = 14 },
    .uart_buf = { .bank = GPIOB, .pin = 2  },
    .uart_tx = { .bank = GPIOC, .pin = 12 },
    .uart_rx = { .bank = GPIOD, .pin = 2  },
    .alt = 8,
};

// Port E - UART10
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_4 = {
    .id1 = { .bank = GPIOC, .pin = 13 },
    .id2 = { .bank = GPIOE, .pin = 12 },
    .uart_buf = { .bank = GPIOB, .pin = 5  },
    .uart_tx = { .bank = GPIOE, .pin = 3  },
    .uart_rx = { .bank = GPIOE, .pin = 2  },
    .alt = 11,
};

// Port F - UART9
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_5 = {
    .id1 = { .bank = GPIOC, .pin = 11 },
    .id2 = { .bank = GPIOE, .pin = 6  },
    .uart_buf = { .bank = GPIOC, .pin = 5  },
    .uart_tx = { .bank = GPIOD, .pin = 15 },
    .uart_rx = { .bank = GPIOD, .pin = 14 },
    .alt = 11,
};

enum {
    UART_PORT_A,
    UART_PORT_B,
    UART_PORT_C,
    UART_PORT_D,
    UART_PORT_E,
    UART_PORT_F,
};

const pbdrv_uart_stm32_hal_platform_data_t
    pbdrv_uart_stm32_hal_platform_data[PBDRV_CONFIG_UART_STM32_HAL_NUM_UART] = {
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
    // [UART_PORT_F] = {
    //     .uart   = UART9,
    //     .irq    = UART9_IRQn,
    // },
};

// overrides weak function in setup.m
void UART4_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_B);
}

// overrides weak function in setup.m
void UART5_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_D);
}

// overrides weak function in setup.m
void UART7_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_A);
}

// overrides weak function in setup.m
void UART8_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_C);
}

// overrides weak function in setup.m
void UART9_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_F);
}

// overrides weak function in setup.m
void UART10_IRQHandler(void) {
    pbdrv_uart_stm32_hal_handle_irq(UART_PORT_E);
}

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
    COUNTER_PORT_C,
    COUNTER_PORT_D,
    COUNTER_PORT_E,
    COUNTER_PORT_F,
};

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
    // [COUNTER_PORT_F] = {
    //     .uart_id    = UART_PORT_F,
    //     .counter_id = COUNTER_PORT_F,
    // },
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

void DMA2_Stream0_IRQHandler() {
    pbdrv_adc_stm32_hal_handle_irq();
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


// Early initialization

// special memory addresses defined in linker script
extern uint32_t *_fw_isr_vector_src;

// Called from assembly code in startup.s
void SystemInit(void) {
    RCC_OscInitTypeDef osc_init;
    RCC_ClkInitTypeDef clk_init;

    // Using external 16Mhz oscillator
    osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc_init.HSEState = RCC_HSE_ON;
    osc_init.HSIState = RCC_HSI_OFF;
    osc_init.PLL.PLLState = RCC_PLL_ON;
    osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc_init.PLL.PLLM = 8; // VCO_IN 2MHz (16MHz / 8)
    osc_init.PLL.PLLN = 96; // VCO_OUT 192MHz (2MHz * 96)
    osc_init.PLL.PLLP = RCC_PLLP_DIV2; // PLLCLK 96MHz (not using max of 100 because of USB)
    osc_init.PLL.PLLQ = 4; // 48MHz USB clock

    HAL_RCC_OscConfig(&osc_init);

    clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;  // HCLK 96MHz
    clk_init.APB1CLKDivider = RCC_HCLK_DIV2; // changed from pyboard since max is 50MHz
    clk_init.APB2CLKDivider = RCC_HCLK_DIV1; // changed from pyboard since max is 100MHz

    HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5);

    // dpgeorge: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
        RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_DMA2EN;
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN | RCC_APB1ENR_UART5EN | RCC_APB1ENR_UART7EN |
        RCC_APB1ENR_UART8EN | RCC_APB1ENR_TIM2EN | RCC_APB1ENR_TIM3EN |
        RCC_APB1ENR_TIM4EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_UART9EN | RCC_APB2ENR_UART10EN |
        RCC_APB2ENR_ADC1EN | RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;

    // Keep main power on (PA13 == POWER_EN)
    GPIOA->BSRR = GPIO_BSRR_BS_13;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER13_Msk) | (1 << GPIO_MODER_MODER13_Pos);

    // Turn VCC_PORT on (PA14 == PORTCE)
    GPIOA->BSRR = GPIO_BSRR_BS_14;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER14_Msk) | (1 << GPIO_MODER_MODER14_Pos);

    // UART for terminal
    // FIXME: temporarily using port F for terminal until we get USB working
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (11 << GPIO_AFRH_AFSEL14_Pos);
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODER15_Msk) | (2 << GPIO_MODER_MODER15_Pos);
    GPIOD->AFR[1] = (GPIOD->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (11 << GPIO_AFRH_AFSEL15_Pos);
    UART9->BRR = HAL_RCC_GetPCLK2Freq() / 115200;
    UART9->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER5_Msk) | (1 << GPIO_MODER_MODER5_Pos);
    GPIOC->BSRR = GPIO_BSRR_BR_5; // TXPORT_F_EN

    // since the firmware starts at 0x08008000, we need to set the vector table offset
    SCB->VTOR = (uint32_t)&_fw_isr_vector_src;

    // bootloader disables interrupts?
    __enable_irq();
}
