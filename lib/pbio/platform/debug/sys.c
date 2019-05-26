// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <string.h>

#include "pbdrv/config.h"
#include "pbdrv/light.h"

#include "pbio/button.h"
#include "pbio/event.h"
#include "pbio/light.h"

#include "pbsys/sys.h"

#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/process.h"

#include "stm32f446xx.h"

// workaround upstream NVIC_SystemReset() not decorated with noreturn
void NVIC_SystemReset(void) __attribute__((noreturn));

typedef enum {
    LED_STATUS_BUTTON_PRESSED   = 1 << 0,
} led_status_flags_t;

// Bitmask of status indicators
static led_status_flags_t led_status_flags;

// values for keeping track of how long button has been pressed
static bool button_pressed;
static clock_time_t button_press_start_time;

// user program stop function
static pbsys_stop_callback_t user_stop_func;
// user program stdin event function
static pbsys_stdin_event_callback_t user_stdin_event_func;

PROCESS(pbsys_process, "System");

void pbsys_prepare_user_program(const pbsys_user_program_callbacks_t *callbacks) {
    if (callbacks) {
        user_stop_func = callbacks->stop;
        user_stdin_event_func = callbacks->stdin_event;
    }
    else {
        user_stop_func = NULL;
        user_stdin_event_func = NULL;
    }
    _pbio_light_set_user_mode(true);
    pbio_light_on_with_pattern(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_GREEN, PBIO_LIGHT_PATTERN_BREATHE);
}

void pbsys_unprepare_user_program(void) {
    uint8_t r, g, b;

    user_stop_func = NULL;
    user_stdin_event_func = NULL;
    _pbio_light_set_user_mode(false);
    pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_BLUE, &r, &g, &b);
    pbdrv_light_set_rgb(PBIO_PORT_SELF, r, g, b);
}

pbio_error_t pbsys_stdin_get_char(uint8_t *c) {
    if (!(USART6->SR & USART_SR_RXNE)) {
        return PBIO_ERROR_AGAIN;
    }

    *c = USART6->DR;

    return PBIO_SUCCESS;
}

pbio_error_t pbsys_stdout_put_char(uint8_t c) {
    if (!(USART6->SR & USART_SR_TXE)) {
        return PBIO_ERROR_AGAIN;
    }
    USART6->DR = c;

    return PBIO_SUCCESS;
}

void pbsys_reboot(bool fw_update) {
    // this function never returns
    NVIC_SystemReset();
}

void pbsys_power_off(void) {
    int i;

    // blink pattern like LEGO firmware
    for (i = 0; i < 3; i++) {
        pbdrv_light_set_rgb(PBIO_PORT_SELF, 255, 140, 60); // white
        clock_delay_usec(50000);
        pbdrv_light_set_rgb(PBIO_PORT_SELF, 0, 0, 0);
        clock_delay_usec(30000);
    }

    // PWM doesn't work while IRQs are disabled? so this needs to be after
    __disable_irq();

    // need to loop because power will stay on as long as button is pressed
    while (true) {
        // can't really cut the power here
    }
}

static void init(void) {
    uint8_t r, g, b;

    _pbio_light_set_user_mode(false);
    pbdrv_light_get_rgb_for_color(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_BLUE, &r, &g, &b);
    pbdrv_light_set_rgb(PBIO_PORT_SELF, r, g, b);
}

static void update_button(clock_time_t now) {
    pbio_button_flags_t btn;

    pbio_button_is_pressed(PBIO_PORT_SELF, &btn);

    if (btn & PBIO_BUTTON_CENTER) {
        if (button_pressed) {

            // if the button is held down for 5 seconds, power off
            if (now - button_press_start_time > clock_from_msec(5000)) {
                // turn off light briefly like official LEGO firmware
                pbdrv_light_set_rgb(PBIO_PORT_SELF, 0, 0, 0);
                for (int i = 0; i < 10; i++) {
                    clock_delay_usec(58000);
                }

                pbsys_power_off();
            }
        }
        else {
            button_press_start_time = now;
            button_pressed = true;
            led_status_flags |= LED_STATUS_BUTTON_PRESSED;
            if (user_stop_func) {
                user_stop_func();
            }
        }
    }
    else {
        button_pressed = false;
        led_status_flags &= ~LED_STATUS_BUTTON_PRESSED;
    }
}

PROCESS_THREAD(pbsys_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    init();
    etimer_set(&timer, clock_from_msec(50));

    while (true) {
        PROCESS_WAIT_EVENT();
        if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
            clock_time_t now = clock_time();
            etimer_reset(&timer);
            update_button(now);
        }
        else if (ev == PBIO_EVENT_COM_CMD) {
            pbio_com_cmd_t cmd = (uint32_t)data;

            switch (cmd) {
            case PBIO_COM_CMD_START_USER_PROGRAM:
                break;
            case PBIO_COM_CMD_STOP_USER_PROGRAM:
                if (user_stop_func) {
                    user_stop_func();
                }
                break;
            }
        }
    }

    PROCESS_END();
}

uint32_t SystemCoreClock = 16000000;

// copied from system_stm32.c in stm32 port
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};

// speci
void SystemInit(void) {
    // basic MCU config
    RCC->CR |= RCC_CR_HSION;
    RCC->CFGR = 0; // reset all
    RCC->CR &= ~(RCC_CR_HSION | RCC_CR_CSSON | RCC_CR_PLLON);
    RCC->PLLCFGR = RCC_PLLCFGR_PLLR_1 | RCC_PLLCFGR_PLLQ_2 | RCC_PLLCFGR_PLLN_7
                 | RCC_PLLCFGR_PLLN_6 | RCC_PLLCFGR_PLLM_4; // reset PLLCFGR
    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CIR = 0; // disable IRQs

    // leave the clock as-is (internal 16MHz)

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
    USART6->BRR = (104 << 4) | 3; // 16MHz/(16*104.1875) = 9598 baud
    USART6->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void assert_failed(uint8_t* file, uint32_t line) {
    // set a breakpoint here for debugging
}
