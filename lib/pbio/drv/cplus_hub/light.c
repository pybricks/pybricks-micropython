// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbdrv/light.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32l431xx.h"

// setup LED PWMs and pins
void _pbdrv_light_init(void) {
    // RGB values are 0-255, so setup timer to allow using those values
    // directly in TIMx->CCRx
    TIM15->PSC = 187;
    TIM15->ARR = 256;
    TIM15->BDTR |= TIM_BDTR_MOE;
    TIM1->PSC = 187;
    TIM1->ARR = 256;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM16->PSC = 187;
    TIM16->ARR = 256;
    TIM16->BDTR |= TIM_BDTR_MOE;

    // red LED on PB15 using TIM15 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE15_Msk) | (2 << GPIO_MODER_MODE15_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (14 << GPIO_AFRH_AFSEL15_Pos);
    TIM15->CCMR1 |= (0x6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; // PWM Mode 1
    TIM15->CCER |= TIM_CCER_CC2E;

    // green LED on PA11 using TIM1 CH4
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE11_Msk) | (2 << GPIO_MODER_MODE11_Pos);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL11_Msk) | (1 << GPIO_AFRH_AFSEL11_Pos);
    TIM1->CCMR2 |= (0x6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE; // PWM Mode 1
    TIM1->CCER |= TIM_CCER_CC4E;

    // blue LED on PA6 using TIM16 CH1
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE6_Msk) | (2 << GPIO_MODER_MODE6_Pos);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~GPIO_AFRL_AFSEL6_Msk) | (14 << GPIO_AFRL_AFSEL6_Pos);
    TIM16->CCMR1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM16->CCER |= TIM_CCER_CC1E;

    TIM15->CR1 |= TIM_CR1_CEN;
    TIM15->EGR |= TIM_EGR_UG;
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM1->EGR |= TIM_EGR_UG;
    TIM16->CR1 |= TIM_CR1_CEN;
    TIM16->EGR |= TIM_EGR_UG;
    TIM15->CCR1 = 0;
    TIM1->CCR3 = 0;
    TIM16->CCR2 = 0;
}

#if PBIO_CONFIG_ENABLE_DEINIT
// turn off the light
void _pbdrv_light_deinit(void) {
    TIM15->CR1 &= ~TIM_CR1_CEN;
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM16->CR1 &= ~TIM_CR1_CEN;
    GPIOB->BSRR = GPIO_BSRR_BR_15;
    GPIOA->BSRR = GPIO_BSRR_BR_11;
    GPIOA->BSRR = GPIO_BSRR_BR_6;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE15_Msk) | (1 << GPIO_MODER_MODE15_Pos);
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE11_Msk) | (1 << GPIO_MODER_MODE11_Pos);
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE6_Msk) | (1 << GPIO_MODER_MODE6_Pos);
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    TIM15->CCR2 = r;
    TIM1->CCR4 = g;
    TIM16->CCR1 = b;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
                                           uint8_t *r, uint8_t *g, uint8_t *b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
    case PBIO_LIGHT_COLOR_NONE:
        *r = 0;
        *g = 0;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_WHITE:
        *r = 185;
        *g = 161;
        *b = 22;
        break;
    case PBIO_LIGHT_COLOR_RED:
        *r = 255;
        *g = 0;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_ORANGE:
        *r = 255;
        *g = 37;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_YELLOW:
        *r = 255;
        *g = 140;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_GREEN:
        *r = 0;
        *g = 255;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_BLUE:
        *r = 0;
        *g = 0;
        *b = 180;
        break;
    case PBIO_LIGHT_COLOR_PURPLE:
        *r = 220;
        *g = 0;
        *b = 110;
        break;
    default:
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}
