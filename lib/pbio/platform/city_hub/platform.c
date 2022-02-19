// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <string.h>

#include <pbdrv/gpio.h>
#include <pbio/button.h>
#include <pbio/uartdev.h>

#include "../../drv/bluetooth/bluetooth_stm32_cc2640.h"
#include "../../drv/button/button_gpio.h"
#include "../../drv/ioport/ioport_lpf2.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/motor_driver/motor_driver_hbridge_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/uart/uart_stm32f0.h"

#include "stm32f030xc.h"

enum {
    COUNTER_PORT_A,
    COUNTER_PORT_B,
};

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
    UART_ID_1,
};

// button

const pbdrv_button_gpio_platform_t pbdrv_button_gpio_platform[PBDRV_CONFIG_BUTTON_GPIO_NUM_BUTTON] = {
    [0] = {
        .gpio = { .bank = GPIOC, .pin = 13 },
        .pull = PBDRV_GPIO_PULL_UP,
        .button = PBIO_BUTTON_CENTER,
        .active_low = true,
    }
};

// Bluetooth

// REVISIT: more of this could be in driver if we enabled HAL on City hub.

// bluetooth address is set at factory at this address
#define FLASH_BD_ADDR ((const uint8_t *)0x08004ffa)

static void bt_spi_init(void) {
    // SPI2 pin mux

    //  nMRDY
    // set PB12 as gpio output high
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER12_Msk) | (1 << GPIO_MODER_MODER12_Pos);
    GPIOB->BSRR = GPIO_BSRR_BS_12;

    // mSRDY
    // set PD2 as gpio input
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODER2_Msk) | (0 << GPIO_MODER_MODER2_Pos);

    // SPI_MOSI
    // set PC3 as SPI2->MOSI
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER3_Msk) | (2 << GPIO_MODER_MODER3_Pos);
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL3_Msk) | (1 << GPIO_AFRL_AFSEL3_Pos);

    // SPI_MISO
    // set PC2 as SPI2->MISO
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER2_Msk) | (2 << GPIO_MODER_MODER2_Pos);
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL2_Msk) | (1 << GPIO_AFRL_AFSEL2_Pos);

    // SPI_SCK
    // set PB13 as SPI2->CLK
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER13_Msk) | (2 << GPIO_MODER_MODER13_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL13_Msk) | (0 << GPIO_AFRH_AFSEL13_Pos);

    // DMA

    DMA1_Channel4->CPAR = (uint32_t)&SPI2->DR;
    DMA1_Channel5->CPAR = (uint32_t)&SPI2->DR;
    DMA1->CSELR = (DMA1->CSELR & ~(DMA_CSELR_C4S | DMA_CSELR_C5S)) | (DMA1_CSELR_CH4_SPI2_RX | DMA1_CSELR_CH5_SPI2_TX);

    NVIC_SetPriority(DMA1_Channel4_5_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

    // SPI2

    // set as master and clock /256
    #define SPI_CR1_BR_DIV256 (SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2)
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV256 | SPI_CR1_SSM;
    // enable DMA rx/tx, chip select, rx not empty irq, 8-bit word size, trigger rx irq on 8-bit
    #define SPI_CR2_DS_8BIT (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2)
    SPI2->CR2 = SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN | SPI_CR2_SSOE | SPI_CR2_RXNEIE | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;

    // LEGO Firmware doesn't do this, but we actually use IRQ for SPI_IRQ pin
    // this is needed since we use __WFI() sometimes
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PD;
    EXTI->IMR |= EXTI_EMR_MR2;
    EXTI->RTSR |= EXTI_RTSR_RT2;
    EXTI->FTSR |= EXTI_FTSR_FT2;
    NVIC_SetPriority(EXTI2_3_IRQn, 3);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
}

static void bt_spi_start_xfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint8_t xfer_size) {
    // hopefully this shouldn't actually block, but we can't disable SPI while
    // it is busy, so just in case...
    while (SPI2->SR & SPI_SR_BSY) {
    }

    // disable the SPI so we can configure it
    SPI2->CR1 &= ~SPI_CR1_SPE;

    // configure DMA
    DMA1_Channel5->CCR = 0;
    DMA1_Channel4->CCR = 0;
    DMA1_Channel5->CMAR = (uint32_t)tx_buf;
    DMA1_Channel5->CNDTR = xfer_size;
    DMA1_Channel4->CMAR = (uint32_t)rx_buf;
    DMA1_Channel4->CNDTR = xfer_size;
    DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_TCIE | DMA_CCR_EN;
    DMA1_Channel5->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_EN;

    // enable SPI to start the xfer
    SPI2->CR1 |= SPI_CR1_SPE;
}

const pbdrv_bluetooth_stm32_cc2640_platform_data_t pbdrv_bluetooth_stm32_cc2640_platform_data = {
    .bd_addr = FLASH_BD_ADDR,
    .reset_gpio = { .bank = GPIOB, .pin = 6 },
    .mrdy_gpio = { .bank = GPIOB, .pin = 12 },
    .spi_init = bt_spi_init,
    .spi_start_xfer = bt_spi_start_xfer,
};

void DMA1_Channel4_5_IRQHandler(void) {
    // if CH4 transfer complete
    if (DMA1->ISR & DMA_ISR_TCIF4) {
        // clear interrupt
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        // disable CH4
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;

        // notify that SPI xfer is complete
        pbdrv_bluetooth_stm32_cc2640_spi_xfer_irq();
    }
}

void EXTI2_3_IRQHandler(void) {
    pbdrv_bluetooth_stm32_cc2640_srdy_irq(!(GPIOD->IDR & GPIO_IDR_2));

    // clear the interrupt
    EXTI->PR = EXTI_PR_PR2;
}

// I/O ports

const pbdrv_ioport_lpf2_platform_data_t pbdrv_ioport_lpf2_platform_data = {
    .port_vcc = { .bank = GPIOB, .pin = 2 },
    .ports = {
        // Port A
        {
            .id1 = { .bank = GPIOA, .pin = 1  },
            .id2 = { .bank = GPIOA, .pin = 3  },
            .uart_buf = { .bank = GPIOB, .pin = 5  },
            .uart_tx = { .bank = GPIOC, .pin = 4  },
            .uart_rx = { .bank = GPIOC, .pin = 5  },
            .alt = 1, // USART3
        },
        // Port B
        {
            .id1 = { .bank = GPIOA, .pin = 0  },
            .id2 = { .bank = GPIOA, .pin = 2  },
            .uart_buf = { .bank = GPIOB, .pin = 4  },
            .uart_tx = { .bank = GPIOC, .pin = 10  },
            .uart_rx = { .bank = GPIOC, .pin = 11 },
            .alt = 0, // USART4
        },
    },
};

// LED

// The constants below are derived from the SunLED XZM2CRKM2DGFBB45SCCB
// datasheet parameters. This is probably the LED used on the Move hub and
// City hub (looks like it anyway). The red brightness factor is multiplied by
// 0.35 compared to the datasheet since it has different resistor value that
// affects the brightness.
static const pbdrv_led_pwm_platform_color_t pbdrv_led_pwm_color = {
    .r_factor = 1000,
    .g_factor = 270,
    .b_factor = 200,
    .r_brightness = 174,
    .g_brightness = 1590,
    .b_brightness = 327,
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_0,
        .r_id = PWM_DEV_2,
        .r_ch = 1,
        .g_id = PWM_DEV_1,
        .g_ch = 1,
        .b_id = PWM_DEV_1,
        .b_ch = 2,
        .scale_factor = 5,
    }
};

// Motor driver

const pbdrv_motor_driver_hbridge_pwm_platform_data_t
    pbdrv_motor_driver_hbridge_pwm_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 9,
        .pin1_alt = 0,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 4,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 7,
        .pin2_alt = 0,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 2,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 8,
        .pin1_alt = 0,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 6,
        .pin2_alt = 0,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 1,
    },
};

// PWM

static void pwm_dev_0_platform_init(void) {
    // port A motor PWM on TIM3 CH2/4
    // port B motor PWM on TIM3 CH1/3
    // the motor driver handles these pins, so nothing to do here
}

static void pwm_dev_1_platform_init(void) {
    // green LED on PB14 using TIM15 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (1 << GPIO_AFRH_AFSEL14_Pos);

    // blue LED on PB15 using TIM15 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (2 << GPIO_MODER_MODER15_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (1 << GPIO_AFRH_AFSEL15_Pos);
}

static void pwm_dev_2_platform_init(void) {
    // red LED on PB8 using TIM16 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (2 << GPIO_AFRH_AFSEL8_Pos);
}

// NOTE: Official LEGO firmware uses 1.2 kHz PWM for motors. We have changed to
// 12 kHz to reduce the unpleasant noise (similar to the frequency used by the
// official EV3 firmware).

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM3,
        .prescalar = 4, // results in 12 MHz clock
        .period = 1000, // 12 MHz divided by 1k makes 12 kHz PWM
        .id = PWM_DEV_0,
        // channel 1/3: Port B motor driver; channel 2/4: Port A motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM15,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12 MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_1,
        // channel 1: Green LED; channel 2: Blue LED
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM16,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12 MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_2,
        // channel 1: Red LED
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
};

// RESET

void pbdrv_reset_power_off(void) {
    // setting PB11 low cuts the power
    GPIOB->BRR = GPIO_BRR_BR_11;
}

// UART

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
    // since the firmware starts at 0x08005000, we need to relocate the
    // interrupt vector table to a place where the CPU knows about it.
    // The space at the start of SRAM is reserved in via the linker script.
    memcpy(_fw_isr_vector_dst, _fw_isr_vector_src, sizeof(_fw_isr_vector_dst));

    // this maps SRAM to 0x00000000
    SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE_Msk) | (3 << SYSCFG_CFGR1_MEM_MODE_Pos);

    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // normally, the system clock would be setup here, but it is already
    // configured by the bootloader, so no need to do it again.

    // SysTick set for 1ms ticks
    SysTick_Config(PBDRV_CONFIG_SYS_CLOCK_RATE / 1000);

    // Enable all of the hardware modules we are using
    RCC->AHBENR |= RCC_AHBENR_DMAEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN
        | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_SPI2EN | RCC_APB1ENR_USART3EN
        | RCC_APB1ENR_USART4EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM15EN;

    // Keep main power on (PB11)
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
    GPIOB->BSRR = GPIO_BSRR_BS_11;
}
