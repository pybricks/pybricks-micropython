// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbdrv/light.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32f446xx.h"

// setup LED PWMs and pins
void _pbdrv_light_init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM4EN | RCC_APB1ENR_TIM12EN;

    // RGB values are 0-255, so setup timer to allow using those values
    // directly in TIMx->CCRx
    TIM12->PSC = 187;
    TIM12->ARR = 256;
    TIM12->BDTR |= TIM_BDTR_MOE;
    TIM3->PSC = 187;
    TIM3->ARR = 256;
    TIM3->BDTR |= TIM_BDTR_MOE;
    TIM4->PSC = 187;
    TIM4->ARR = 256;
    TIM4->BDTR |= TIM_BDTR_MOE;

    // red LED LD3 on PB14 using TIM12 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (9 << GPIO_AFRH_AFSEL14_Pos);
    TIM12->CCMR1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM12->CCER |= TIM_CCER_CC1E;

    // green LED LD1 on PB0 using TIM3 CH3
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER0_Msk) | (2 << GPIO_MODER_MODER0_Pos);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL0_Msk) | (2 << GPIO_AFRL_AFSEL0_Pos);
    TIM3->CCMR2 |= (0x6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE; // PWM Mode 1
    TIM3->CCER |= TIM_CCER_CC3E;

    // blue LED LD2 on PB7 using TIM4 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~GPIO_AFRL_AFSEL7_Msk) | (2 << GPIO_AFRL_AFSEL7_Pos);
    TIM4->CCMR1 |= (0x6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; // PWM Mode 1
    TIM4->CCER |= TIM_CCER_CC2E;

    TIM12->CR1 |= TIM_CR1_CEN;
    TIM12->EGR |= TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
    TIM3->EGR |= TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;
    TIM4->EGR |= TIM_EGR_UG;
    TIM12->CCR1 = 0;
    TIM3->CCR3 = 0;
    TIM4->CCR2 = 0;
}

#if PBIO_CONFIG_ENABLE_DEINIT
// turn off the light
void _pbdrv_light_deinit(void) {
    TIM12->CR1 &= ~TIM_CR1_CEN;
    TIM3->CR1 &= ~TIM_CR1_CEN;
    TIM4->CR1 &= ~TIM_CR1_CEN;
    GPIOB->BSRR = GPIO_BSRR_BR_14;
    GPIOB->BSRR = GPIO_BSRR_BR_0;
    GPIOB->BSRR = GPIO_BSRR_BR_7;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (1 << GPIO_MODER_MODER14_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER0_Msk) | (1 << GPIO_MODER_MODER0_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    TIM12->CCR1 = r;
    TIM3->CCR3 = g;
    TIM4->CCR2 = b;

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
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}
