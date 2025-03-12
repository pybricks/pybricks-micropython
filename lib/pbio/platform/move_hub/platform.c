// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <string.h>

#include <pbdrv/gpio.h>
#include <pbdrv/ioport.h>
#include <pbio/button.h>
#include <pbio/port_interface.h>

#include "../../drv/button/button_gpio.h"
#include "../../drv/counter/counter_stm32f0_gpio_quad_enc.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/motor_driver/motor_driver_hbridge_pwm.h"
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

// button

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
        .counter_id = COUNTER_PORT_A,
    },
    [1] = {
        .gpio_int = { .bank = GPIOA, .pin = 0},
        .gpio_dir = { .bank = GPIOA, .pin = 1},
        .counter_id = COUNTER_PORT_B,
    },
};

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = GPIOB,
    .pin = 2
};

const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .uart_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = NULL,
        .supported_modes = PBIO_PORT_MODE_QUADRATURE_PASSIVE,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .uart_driver_index = UART_ID_0,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOB, .pin = 7  },
            .p6 = { .bank = GPIOC, .pin = 15 },
            .uart_buf = { .bank = GPIOB, .pin = 4  },
            .uart_tx = { .bank = GPIOC, .pin = 10 },
            .uart_rx = { .bank = GPIOC, .pin = 11 },
            .uart_tx_alt_uart = 0, // USART4
            .uart_rx_alt_uart = 0, // USART4
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_PUP | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .uart_driver_index = UART_ID_1,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOB, .pin = 10 },
            .p6 = { .bank = GPIOA, .pin = 12 },
            .uart_buf = { .bank = GPIOB, .pin = 0  },
            .uart_tx = { .bank = GPIOC, .pin = 4  },
            .uart_rx = { .bank = GPIOC, .pin = 5  },
            .uart_tx_alt_uart = 1, // USART3
            .uart_rx_alt_uart = 1, // USART3
        },

        .supported_modes = PBIO_PORT_MODE_LEGO_PUP | PBIO_PORT_MODE_UART,
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
        .r_id = PWM_DEV_3,
        .r_ch = 1,
        .g_id = PWM_DEV_2,
        .g_ch = 1,
        .b_id = PWM_DEV_2,
        .b_ch = 2,
        .scale_factor = 5,
    }
};

// Motor driver

const pbdrv_motor_driver_hbridge_pwm_platform_data_t
    pbdrv_motor_driver_hbridge_pwm_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 8,
        .pin1_alt = 2,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOA,
        .pin2_gpio.pin = 10,
        .pin2_alt = 2,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 3,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 9,
        .pin1_alt = 2,
        .pin1_pwm_id = PWM_DEV_0,
        .pin1_pwm_ch = 2,
        .pin2_gpio.bank = GPIOA,
        .pin2_gpio.pin = 11,
        .pin2_alt = 2,
        .pin2_pwm_id = PWM_DEV_0,
        .pin2_pwm_ch = 4,
    },
    // Port C
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 8,
        .pin1_alt = 0,
        .pin1_pwm_id = PWM_DEV_1,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 6,
        .pin2_alt = 0,
        .pin2_pwm_id = PWM_DEV_1,
        .pin2_pwm_ch = 1,
    },
    // Port D
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 7,
        .pin1_alt = 0,
        .pin1_pwm_id = PWM_DEV_1,
        .pin1_pwm_ch = 2,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 9,
        .pin2_alt = 0,
        .pin2_pwm_id = PWM_DEV_1,
        .pin2_pwm_ch = 4,
    },
};

// PWM

static void pwm_dev_0_platform_init(void) {
    // port A: PA8, PA10 - port B: PA9, PA11
    // pin mux handled by motor driver
}

static void pwm_dev_1_platform_init(void) {
    // port C: PC6, PC8 - port D: PC7, PC9
    // pin mux handled by motor driver
}

static void pwm_dev_2_platform_init(void) {
    // green LED on PB14 using TIM15 CH1
    pbdrv_gpio_t gpio = { .bank = GPIOB, .pin = 14 };
    pbdrv_gpio_alt(&gpio, 1);

    // blue LED on PB15 using TIM15 CH2
    gpio.pin = 15;
    pbdrv_gpio_alt(&gpio, 1);
}

static void pwm_dev_3_platform_init(void) {
    // red LED on PB8 using TIM16 CH1
    pbdrv_gpio_t gpio = { .bank = GPIOB, .pin = 8 };
    pbdrv_gpio_alt(&gpio, 2);
}

// NOTE: Official LEGO firmware uses 1.2 kHz PWM for motors. We have changed to
// 12 kHz to reduce the unpleasant noise (similar to the frequency used by the
// official EV3 firmware).

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM1,
        .prescalar = 4, // results in 12 MHz clock
        .period = 1000, // 12 MHz divided by 1k makes 12 kHz PWM
        .id = PWM_DEV_0,
        // channel 1/3: Port A motor driver; channel 2/4: Port B motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM3,
        .prescalar = 4, // results in 12 MHz clock
        .period = 1000, // 12 MHz divided by 1k makes 12 kHz PWM
        .id = PWM_DEV_1,
        // channel 1/3: Port C motor driver; channel 3/4: Port D motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM15,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12 MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_2,
        // channel 1: Green LED; channel 2: Blue LED
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE,
    },
    {
        .platform_init = pwm_dev_3_platform_init,
        .TIMx = TIM16,
        .prescalar = 4, // results in 12 MHz clock
        .period = 10000, // 12 MHz divided by 10k makes 1.2 kHz PWM
        .id = PWM_DEV_3,
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
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN
        | RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN | RCC_AHBENR_DMAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_USART3EN | RCC_APB1ENR_USART4EN
        | RCC_APB1ENR_SPI2EN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN | RCC_APB2ENR_TIM1EN
        | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_TIM16EN | RCC_APB2ENR_SPI1EN;

    // Keep main power on (PB11)
    pbdrv_gpio_t power_gpio = { .bank = GPIOB, .pin = 11 };
    pbdrv_gpio_out_high(&power_gpio);
}
