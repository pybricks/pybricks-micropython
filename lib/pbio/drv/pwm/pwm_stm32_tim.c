// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// PWM driver using TIM devices on STM32 MCUs.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_STM32_TIM

#include <stdint.h>

#include STM32_H

#include <pbdrv/pwm.h>
#include <pbio/error.h>

#include "./pwm_stm32_tim.h"

static pbio_error_t pbdrv_pwm_stm32_tim_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    const pbdrv_pwm_stm32_tim_platform_data_t *pdata = dev->pdata;
    TIM_TypeDef *TIMx = pdata->TIMx;

    switch (ch) {
        case 1:
            TIMx->CCR1 = value;
            break;
        case 2:
            TIMx->CCR2 = value;
            break;
        case 3:
            TIMx->CCR3 = value;
            break;
        case 4:
            TIMx->CCR4 = value;
            break;
    }

    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_stm32_tim_funcs = {
    .set_duty = pbdrv_pwm_stm32_tim_set_duty,
};

void pbdrv_pwm_stm32_tim_init(pbdrv_pwm_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV; i++) {
        const pbdrv_pwm_stm32_tim_platform_data_t *pdata = &pbdrv_pwm_stm32_tim_platform_data[i];
        pbdrv_pwm_dev_t *dev = &devs[pdata->id];
        TIM_TypeDef *TIMx = pdata->TIMx;

        TIMx->PSC = pdata->prescalar - 1;
        TIMx->ARR = pdata->period;
        TIMx->BDTR |= TIM_BDTR_MOE;

        uint32_t ccmr1 = TIMx->CCMR1;
        uint32_t ccmr2 = TIMx->CCMR2;
        uint32_t ccer = TIMx->CCER;

        if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE) {
            ccmr1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
            ccer |= TIM_CCER_CC1E;
            #if PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT) {
                ccer |= TIM_CCER_CC1P;
            }
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_1_COMPLEMENT) {
                ccer |= TIM_CCER_CC1NE;
            }
            #endif // PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
        }

        if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE) {
            ccmr1 |= (0x6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;
            ccer |= TIM_CCER_CC2E;
            #if PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT) {
                ccer |= TIM_CCER_CC2P;
            }
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_2_COMPLEMENT) {
                ccer |= TIM_CCER_CC2NE;
            }
            #endif // PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
        }

        if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE) {
            ccmr2 |= (0x6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;
            ccer |= TIM_CCER_CC3E;
            #if PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT) {
                ccer |= TIM_CCER_CC3P;
            }
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_3_COMPLEMENT) {
                ccer |= TIM_CCER_CC3NE;
            }
            #endif // PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
        }

        if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE) {
            ccmr2 |= (0x6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;
            ccer |= TIM_CCER_CC4E;
            #if PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
            if (pdata->channels & PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT) {
                ccer |= TIM_CCER_CC4P;
            }
            #endif // PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
        }

        TIMx->CCMR1 = ccmr1;
        TIMx->CCMR2 = ccmr2;
        TIMx->CCER = ccer;

        pdata->platform_init();

        TIMx->CR1 |= TIM_CR1_CEN;
        TIMx->EGR |= TIM_EGR_UG;

        dev->pdata = pdata;
        dev->funcs = &pbdrv_pwm_stm32_tim_funcs;
    }
}

#endif // PBDRV_CONFIG_PWM_STM32_TIM
