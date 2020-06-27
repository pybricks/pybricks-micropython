// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <string.h>

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/uartdev.h>

#include "../../drv/button/button_gpio.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/uart/uart_stm32f0.h"

#include "stm32f030xc.h"

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = { .bank = GPIOC, .pin = 13 },
        .pull = PBDRV_GPIO_PULL_UP,
        .button = PBIO_BUTTON_CENTER,
        .active_low = true,
    }
};

// Port A - USART3
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_0 = {
    .id1 = { .bank = GPIOA, .pin = 1  },
    .id2 = { .bank = GPIOA, .pin = 3  },
    .uart_buf = { .bank = GPIOB, .pin = 5  },
    .uart_tx = { .bank = GPIOC, .pin = 4  },
    .uart_rx = { .bank = GPIOC, .pin = 5  },
    .alt = 1,
};

// Port B - USART4
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_1 = {
    .id1 = { .bank = GPIOA, .pin = 0  },
    .id2 = { .bank = GPIOA, .pin = 2  },
    .uart_buf = { .bank = GPIOB, .pin = 4  },
    .uart_tx = { .bank = GPIOC, .pin = 10  },
    .uart_rx = { .bank = GPIOC, .pin = 11 },
    .alt = 0,
};

// UART

enum {
    UART_ID_0,
    UART_ID_1,
};

const pbdrv_uart_stm32f0_platform_data_t pbdrv_uart_stm32f0_platform_data[PBDRV_CONFIG_UART_STM32F0_NUM_UART] = {
    [UART_ID_0] = {
        .uart = USART3,
        .irq = USART3_6_IRQn,
    },
    [UART_ID_1] = {
        .uart = USART4,
        .irq = USART3_6_IRQn,
    },
};

// overrides weak function in setup_*.m
void USART3_6_IRQHandler(void) {
    pbdrv_uart_stm32f0_handle_irq(UART_ID_0);
    pbdrv_uart_stm32f0_handle_irq(UART_ID_1);
}

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
};

const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id = UART_ID_0,
        .counter_id = COUNTER_PORT_A,
    },
    [1] = {
        .uart_id = UART_ID_1,
        .counter_id = COUNTER_PORT_B,
    },
};

// special memory addresses defined in linker script
extern uint32_t _fw_isr_vector_src[48];
extern uint32_t _fw_isr_vector_dst[48];

// Called from assembly code in startup_stm32f070xb.s
// this function is a mash up of ports/stm32/system_stm32f0.c from MicroPython
// and the official LEGO firmware
void SystemInit(void) {
    // normally, the system clock would be setup here, but it is already
    // configured by the bootloader, so no need to do it again.

    // dpgeorge: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // Enable all of the shared hardware modules we are using

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN
        | RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN | RCC_APB1ENR_USART4EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;


    // Keep main power on
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
    GPIOB->BSRR = GPIO_BSRR_BS_11;

    // Turn on port VCC
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER2_Msk) | (1 << GPIO_MODER_MODER2_Pos);
    GPIOB->BSRR = GPIO_BSRR_BS_2;

    // not sure what the rest of these pins do

    // PA5 output, low
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER5_Msk) | (1 << GPIO_MODER_MODER5_Pos);
    GPIOA->BSRR = GPIO_BSRR_BR_5;

    // PA15 output, low
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);
    GPIOA->BSRR = GPIO_BSRR_BR_15;

    // PB1 output, low
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER1_Msk) | (1 << GPIO_MODER_MODER1_Pos);
    GPIOB->BSRR = GPIO_BSRR_BR_1;

    // PB7 output, low
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
    GPIOB->BSRR = GPIO_BSRR_BR_7;

    // PB9 output, low
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOB->BSRR = GPIO_BSRR_BR_9;

    // PB10 output, low
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
    GPIOB->BSRR = GPIO_BSRR_BR_10;

    // PC14 output, low
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER14_Msk) | (1 << GPIO_MODER_MODER14_Pos);
    GPIOC->BSRR = GPIO_BSRR_BR_14;

    // PC15 output, low
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);
    GPIOC->BSRR = GPIO_BSRR_BR_15;

    // PF0 output, low
    GPIOF->MODER = (GPIOF->MODER & ~GPIO_MODER_MODER0_Msk) | (1 << GPIO_MODER_MODER0_Pos);
    GPIOF->BSRR = GPIO_BSRR_BR_0;

    // PF1 output, low
    GPIOF->MODER = (GPIOF->MODER & ~GPIO_MODER_MODER1_Msk) | (1 << GPIO_MODER_MODER1_Pos);
    GPIOF->BSRR = GPIO_BSRR_BR_1;

    // since the firmware starts at 0x08005000, we need to relocate the
    // interrupt vector table to a place where the CPU knows about it.
    // The space at the start of SRAM is reserved in via the linker script.
    memcpy(_fw_isr_vector_dst, _fw_isr_vector_src, sizeof(_fw_isr_vector_dst));

    // this maps SRAM to 0x00000000
    SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE_Msk) | (3 << SYSCFG_CFGR1_MEM_MODE_Pos);
}
