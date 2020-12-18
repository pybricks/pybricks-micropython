// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Sound driver using DAC on STM32 MCU.

#ifndef _INTERNAL_PBDRV_SOUND_STM32_HAL_DAC_H_
#define _INTERNAL_PBDRV_SOUND_STM32_HAL_DAC_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_STM32_HAL_DAC

#include <stdint.h>

#include STM32_H

typedef struct {
    /** GPIO bank for amplifier enable signal. */
    GPIO_TypeDef *enable_gpio_bank;
    /** GPIO ping for amplifier enable signal (one of HAL GPIO_pins_define). */
    uint32_t enable_gpio_pin;
    /** The DAC peripheral. */
    DAC_TypeDef *dac;
    /** The DAC channel (one of HAL DAC_Channel_selection). */
    uint32_t dac_ch;
    /** The DAC trigger source (one of HAL DAC_trigger_selection). */
    uint32_t dac_trigger;
    /** The timer peripheral. */
    TIM_TypeDef *tim;
    /** Clock rate supplied to timer (Hz). */
    uint32_t tim_clock_rate;
    /** The DMA peripheral. */
    DMA_Stream_TypeDef *dma;
    /** The DMA channel (one of HAL DMA_Channel_selection). */
    uint32_t dma_ch;
    /** The DMA interrupt. */
    IRQn_Type dma_irq;
} pbdrv_sound_stm32_hal_dac_platform_data_t;

/** Platform-specific data - defined in platform.c */
extern const pbdrv_sound_stm32_hal_dac_platform_data_t pbdrv_sound_stm32_hal_dac_platform_data;

void pbdrv_sound_stm32_hal_dac_handle_dma_irq(void);

#endif // PBDRV_CONFIG_SOUND_STM32_HAL_DAC

#endif // _INTERNAL_PBDRV_SOUND_STM32_HAL_DAC_H_
