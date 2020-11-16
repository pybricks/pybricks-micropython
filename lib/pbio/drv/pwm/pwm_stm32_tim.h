// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// PWM driver using TIM devices on STM32 MCUs.

#ifndef _PBDRV_PWM_STM32_TIM_H_
#define _PBDRV_PWM_STM32_TIM_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_STM32_TIM

#include <stdint.h>

#include STM32_H

#include <pbdrv/pwm.h>

/** Bit flags representing PWM channel options. */
typedef enum {
    PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE = 1 << 0,
    PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE = 1 << 1,
    PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE = 1 << 2,
    PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE = 1 << 3,
    // These are optional to reduce firmware size on small hubs
    #if PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
    PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT = 1 << 4,
    PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT = 1 << 5,
    PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT = 1 << 6,
    PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT = 1 << 7,
    PBDRV_PWM_STM32_TIM_CHANNEL_1_COMPLEMENT = 1 << 8,
    PBDRV_PWM_STM32_TIM_CHANNEL_2_COMPLEMENT = 1 << 9,
    PBDRV_PWM_STM32_TIM_CHANNEL_3_COMPLEMENT = 1 << 10,
    #endif // PBDRV_CONFIG_PWM_STM32_TIM_EXTRA_FLAGS
} pbdrv_pwm_stm32_tim_channel_t;

/** Platform-specific device information. */
typedef struct {
    /** Callback for platform-specific initialization (e.g. pin mux) */
    void (*platform_init)(void);
    /** The timer peripheral to use */
    TIM_TypeDef *TIMx;
    /** Clock prescalar value. */
    uint16_t prescalar;
    /** Period in terms of counts. */
    uint16_t period;
    /* Bitmask of channels options to enable. */
    pbdrv_pwm_stm32_tim_channel_t channels;
    /** Unique ID (array index) for this instance. */
    uint8_t id;
} pbdrv_pwm_stm32_tim_platform_data_t;

// Defined in platform.c
extern const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV];

void pbdrv_pwm_stm32_tim_init(pbdrv_pwm_dev_t *dev);
void pbdrv_pwm_stm32_tim_deinit(pbdrv_pwm_dev_t *dev);

#else // PBDRV_CONFIG_PWM_STM32_TIM

#define pbdrv_pwm_stm32_tim_init(dev)
#define pbdrv_pwm_stm32_tim_deinit(dev)

#endif // PBDRV_CONFIG_PWM_STM32_TIM

#endif // _PBDRV_PWM_STM32_TIM_H_
