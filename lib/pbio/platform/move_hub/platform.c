// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <string.h>

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/uartdev.h>

#include "../../drv/button/button_gpio.h"
#include "../../drv/counter/counter_stm32f0_gpio_quad_enc.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/uart/uart_stm32f0.h"

#include "stm32f070xb.h"

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
    COUNTER_PORT_C,
    COUNTER_PORT_D,
};

enum {
    LED_DEV_0,
};

enum {
    PWM_DEV_0,
    PWM_DEV_1,
    PWM_DEV_2,
    PWM_DEV_3,
};

enum {
    UART_ID_0,
    UART_ID_1,
};

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = { .bank = GPIOC, .pin = 13 },
        .pull = PBDRV_GPIO_PULL_UP,
        .button = PBIO_BUTTON_CENTER,
        .active_low = true,
    },
};

// counter devices

const pbdrv_counter_stm32f0_gpio_quad_enc_platform_data_t
    pbdrv_counter_stm32f0_gpio_quad_enc_platform_data[] = {
    [0] = {
        .gpio_int = { .bank = GPIOB, .pin = 1},
        .gpio_dir = { .bank = GPIOB, .pin = 9},
        .invert = true,
        .counter_id = COUNTER_PORT_A,
    },
    [1] = {
        .gpio_int = { .bank = GPIOA, .pin = 0},
        .gpio_dir = { .bank = GPIOA, .pin = 1},
        .invert = false,
        .counter_id = COUNTER_PORT_B,
    },
};

// LED

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .id = LED_DEV_0,
        .r_id = PWM_DEV_3,
        .r_ch = 1,
        .g_id = PWM_DEV_2,
        .g_ch = 1,
        .b_id = PWM_DEV_2,
        .b_ch = 2,
        .scale_factor = 4,
    }
};

// PWM

static void pwm_dev_0_platform_init() {
    // port A
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOA->BRR = GPIO_BRR_BR_8;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
    GPIOA->BRR = GPIO_BRR_BR_10;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (2 << GPIO_AFRH_AFSEL8_Pos);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL10_Msk) | (2 << GPIO_AFRH_AFSEL10_Pos);

    // port B
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOA->BRR = GPIO_BRR_BR_9;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
    GPIOA->BRR = GPIO_BRR_BR_11;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) | (2 << GPIO_AFRH_AFSEL9_Pos);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL11_Msk) | (2 << GPIO_AFRH_AFSEL11_Pos);
}

static void pwm_dev_1_platform_init() {
    // port C
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
    GPIOC->BRR = GPIO_BRR_BR_6;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOC->BRR = GPIO_BRR_BR_8;
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL6_Msk) | (0 << GPIO_AFRL_AFSEL6_Pos);
    GPIOC->AFR[1] = (GPIOC->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (0 << GPIO_AFRH_AFSEL8_Pos);

    // port D
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
    GPIOC->BRR = GPIO_BRR_BR_7;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOC->BRR = GPIO_BRR_BR_9;
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL7_Msk) | (0 << GPIO_AFRL_AFSEL7_Pos);
    GPIOC->AFR[1] = (GPIOC->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) | (0 << GPIO_AFRH_AFSEL9_Pos);
}

static void pwm_dev_2_platform_init() {
    // green LED on PB14 using TIM15 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (1 << GPIO_AFRH_AFSEL14_Pos);

    // blue LED on PB15 using TIM15 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (2 << GPIO_MODER_MODER15_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (1 << GPIO_AFRH_AFSEL15_Pos);
}

static void pwm_dev_3_platform_init() {
    // red LED on PB8 using TIM16 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (2 << GPIO_AFRH_AFSEL8_Pos);
}

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM1,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_0,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM3,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_1,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM15,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_2,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE,
    },
    {
        .platform_init = pwm_dev_3_platform_init,
        .TIMx = TIM16,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_3,
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
};

// RESET

void pbdrv_reset_stm32_power_off() {
    // setting PB11 low cuts the power
    GPIOB->BRR = GPIO_BRR_BR_11;
}

// Port C - USART4
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_0 = {
    .id1 = { .bank = GPIOB, .pin = 7  },
    .id2 = { .bank = GPIOC, .pin = 15 },
    .uart_buf = { .bank = GPIOB, .pin = 4  },
    .uart_tx = { .bank = GPIOC, .pin = 10 },
    .uart_rx = { .bank = GPIOC, .pin = 11 },
    .alt = 0,
};

// Port D - USART3
const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_1 = {
    .id1 = { .bank = GPIOB, .pin = 10 },
    .id2 = { .bank = GPIOA, .pin = 12 },
    .uart_buf = { .bank = GPIOB, .pin = 0  },
    .uart_tx = { .bank = GPIOC, .pin = 4  },
    .uart_rx = { .bank = GPIOC, .pin = 5  },
    .alt = 1,
};

// UART

const pbdrv_uart_stm32f0_platform_data_t pbdrv_uart_stm32f0_platform_data[PBDRV_CONFIG_UART_STM32F0_NUM_UART] = {
    [UART_ID_0] = {
        .uart = USART4,
        .irq = USART3_4_IRQn,
    },
    [UART_ID_1] = {
        .uart = USART3,
        .irq = USART3_4_IRQn,
    },
};

// overrides weak function in setup_*.m
void USART3_4_IRQHandler(void) {
    pbdrv_uart_stm32f0_handle_irq(UART_ID_0);
    pbdrv_uart_stm32f0_handle_irq(UART_ID_1);
}

#if PBIO_CONFIG_UARTDEV
const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV] = {
    [0] = {
        .uart_id = UART_ID_0,
        .counter_id = COUNTER_PORT_C,
    },
    [1] = {
        .uart_id = UART_ID_1,
        .counter_id = COUNTER_PORT_D,
    },
};
#endif // PBIO_CONFIG_UARTDEV

// special memory addresses defined in linker script
extern uint32_t _fw_isr_vector_src[48];
extern uint32_t _fw_isr_vector_dst[48];

// Called from assembly code in startup_stm32f070xb.s
// this function is a mash up of ports/stm32/system_stm32f0.c from MicroPython
// and the official LEGO firmware
void SystemInit(void) {
    // normally, the system clock would be setup here, but it is already
    // configured by the bootloader, so no need to do it again.

    // SysTick set for 1ms ticks
    SysTick_Config(PBDRV_CONFIG_SYS_CLOCK_RATE / 1000);

    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // Enable all of the hardware modules we are using
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN
        | RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_USART3EN | RCC_APB1ENR_USART4EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN | RCC_APB2ENR_TIM1EN
        | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN;

    // Keep BOOST alive
    GPIOB->BSRR = GPIO_BSRR_BS_11;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);

    // PB2 controls I/O port VCC
    GPIOB->BSRR = GPIO_BSRR_BS_2;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER2_Msk) | (1 << GPIO_MODER_MODER2_Pos);

    // not sure what the rest of these pins do

    // PF0 output, high
    GPIOF->BSRR = GPIO_BSRR_BS_0;
    GPIOF->MODER = (GPIOF->MODER & ~GPIO_MODER_MODER0_Msk) | (1 << GPIO_MODER_MODER0_Pos);

    // PA15 output, high
    GPIOA->BSRR = GPIO_BSRR_BS_15;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);

    // PB5 output, high
    GPIOB->BSRR = GPIO_BSRR_BS_5;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER5_Msk) | (1 << GPIO_MODER_MODER5_Pos);

    // PC12 output, high
    GPIOC->BSRR = GPIO_BSRR_BS_12;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER12_Msk) | (1 << GPIO_MODER_MODER12_Pos);

    // PD2 output, high
    GPIOD->BSRR = GPIO_BSRR_BS_2;
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODER2_Msk) | (1 << GPIO_MODER_MODER2_Pos);

    // PF1 output, high
    GPIOF->BSRR = GPIO_BSRR_BS_1;
    GPIOF->MODER = (GPIOF->MODER & ~GPIO_MODER_MODER1_Msk) | (1 << GPIO_MODER_MODER1_Pos);

    // since the firmware starts at 0x08005000, we need to relocate the
    // interrupt vector table to a place where the CPU knows about it.
    // The space at the start of SRAM is reserved in via the linker script.
    memcpy(_fw_isr_vector_dst, _fw_isr_vector_src, sizeof(_fw_isr_vector_dst));

    // this maps SRAM to 0x00000000
    SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE_Msk) | (3 << SYSCFG_CFGR1_MEM_MODE_Pos);
}
