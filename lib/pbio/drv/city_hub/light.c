// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LIGHT

#include <pbdrv/light.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32f030xc.h"

// setup LED PWMs and pins
void _pbdrv_light_init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM15EN;

    // RGB values are 0-255, so multiplying by 5 here to limit brightness to
    // 1/5 of max possible without having to do division later. It should also
    // give use smoother steps than the official LEGO firmware since we aren't
    // jumping by 5s.
    TIM15->PSC = 187;
    TIM15->ARR = 256 * 5;
    TIM15->BDTR |= TIM_BDTR_MOE;
    TIM16->PSC = 187;
    TIM16->ARR = 256 * 5;
    TIM16->BDTR |= TIM_BDTR_MOE;

    // red LED on PB8 using TIM16 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (2 << GPIO_AFRH_AFSEL8_Pos);
    TIM16->CCMR1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM16->CCER |= TIM_CCER_CC1E;

    // green LED on PB14 using TIM15 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (1 << GPIO_AFRH_AFSEL14_Pos);
    TIM15->CCMR1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM15->CCER |= TIM_CCER_CC1E;

    // blue LED on PB15 using TIM15 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (2 << GPIO_MODER_MODER15_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (1 << GPIO_AFRH_AFSEL15_Pos);
    TIM15->CCMR1 |= (0x6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; // PWM Mode 1
    TIM15->CCER |= TIM_CCER_CC2E;

    TIM16->CR1 |= TIM_CR1_CEN;
    TIM16->EGR |= TIM_EGR_UG;
    TIM15->CR1 |= TIM_CR1_CEN;
    TIM15->EGR |= TIM_EGR_UG;
    TIM16->CCR1 = 0;
    TIM15->CCR1 = 0;
    TIM15->CCR2 = 0;
}

#if PBIO_CONFIG_ENABLE_DEINIT
// turn off the light
void _pbdrv_light_deinit(void) {
    TIM15->CR1 &= ~TIM_CR1_CEN;
    TIM16->CR1 &= ~TIM_CR1_CEN;
    GPIOB->BRR = GPIO_BRR_BR_8;
    GPIOB->BRR = GPIO_BRR_BR_14;
    GPIOB->BRR = GPIO_BRR_BR_15;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (1 << GPIO_MODER_MODER14_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port == PBIO_PORT_B || port == PBIO_PORT_A) {
        // TODO: check for Powered UP Lights connected to ports C/D
        return PBIO_ERROR_NO_DEV;
    }

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    TIM16->CCR1 = r;
    TIM15->CCR1 = g;
    TIM15->CCR2 = b;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
    uint8_t *r, uint8_t *g, uint8_t *b) {
    if (port == PBIO_PORT_B || port == PBIO_PORT_A) {
        // TODO: check for Powered UP Lights connected to ports C/D
        return PBIO_ERROR_NO_DEV;
    }

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
        case PBIO_LIGHT_COLOR_WHITE:
            *r = 255;
            *g = 140;
            *b = 60;
            break;
        case PBIO_LIGHT_COLOR_RED:
            *r = 255;
            *g = 0;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_ORANGE:
            *r = 255;
            *g = 25;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_YELLOW:
            *r = 255;
            *g = 70;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_GREEN:
            *r = 0;
            *g = 200;
            *b = 0;
            break;
        case PBIO_LIGHT_COLOR_BLUE:
            *r = 0;
            *g = 0;
            *b = 255;
            break;
        case PBIO_LIGHT_COLOR_PURPLE:
            *r = 220;
            *g = 0;
            *b = 120;
            break;
        default:
            *r = 0;
            *g = 0;
            *b = 0;
            break;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_LIGHT
