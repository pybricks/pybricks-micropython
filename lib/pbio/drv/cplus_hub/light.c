// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LIGHT

#include <pbdrv/light.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include "stm32l4xx_hal.h"

extern TIM_HandleTypeDef *cplus_hub_htim1;
extern TIM_HandleTypeDef *cplus_hub_htim15;
extern TIM_HandleTypeDef *cplus_hub_htim16;

// setup LED PWMs and pins
void _pbdrv_light_init(void) {
    TIM_OC_InitTypeDef tim_oc_init = { 0 };

    tim_oc_init.OCMode = TIM_OCMODE_PWM1;
    tim_oc_init.OCPolarity = TIM_OCPOLARITY_LOW;

    // red LED on PB15 using TIM15 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE15_Msk) | (2 << GPIO_MODER_MODE15_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (14 << GPIO_AFRH_AFSEL15_Pos);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim15, &tim_oc_init, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(cplus_hub_htim15, TIM_CHANNEL_2);

    // green LED on PA11 using TIM1 CH4
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE11_Msk) | (2 << GPIO_MODER_MODE11_Pos);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL11_Msk) | (1 << GPIO_AFRH_AFSEL11_Pos);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim1, &tim_oc_init, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(cplus_hub_htim1, TIM_CHANNEL_4);

    // blue LED on PA6 using TIM16 CH1
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE6_Msk) | (2 << GPIO_MODER_MODE6_Pos);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~GPIO_AFRL_AFSEL6_Msk) | (14 << GPIO_AFRL_AFSEL6_Pos);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim16, &tim_oc_init, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(cplus_hub_htim16, TIM_CHANNEL_1);
}

#if PBIO_CONFIG_ENABLE_DEINIT
// turn off the light
void _pbdrv_light_deinit(void) {
    HAL_TIM_PWM_Stop(cplus_hub_htim16, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(cplus_hub_htim1, TIM_CHANNEL_4);
    HAL_TIM_PWM_Stop(cplus_hub_htim15, TIM_CHANNEL_2);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE15_Msk) | (1 << GPIO_MODER_MODE15_Pos);
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE11_Msk) | (1 << GPIO_MODER_MODE11_Pos);
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODE6_Msk) | (1 << GPIO_MODER_MODE6_Pos);
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Timers have period of 10000 and we want period / 5 as max brightness.

    TIM15->CCR2 = 10000 - r * 2000 / 255;
    TIM1->CCR4 = 10000 - g * 2000 / 255;
    TIM16->CCR1 = 10000 - b * 2000 / 255;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
                                           uint8_t *r, uint8_t *g, uint8_t *b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
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
        *r = 0;
        *g = 0;
        *b = 0;
        break;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_LIGHT
