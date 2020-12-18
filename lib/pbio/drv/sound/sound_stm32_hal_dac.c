// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Sound driver using DAC on STM32 MCU.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_STM32_HAL_DAC

#include <stdint.h>

#include "sound_stm32_hal_dac.h"

#include STM32_HAL_H

static DMA_HandleTypeDef pbdrv_sound_hdma;
static DAC_HandleTypeDef pbdrv_sound_hdac;
static TIM_HandleTypeDef pbdrv_sound_htim;

void pbdrv_sound_init(void) {
    const pbdrv_sound_stm32_hal_dac_platform_data_t *pdata = &pbdrv_sound_stm32_hal_dac_platform_data;

    GPIO_InitTypeDef gpio_init;
    gpio_init.Pin = pdata->enable_gpio_pin;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(pdata->enable_gpio_bank, &gpio_init);
    HAL_GPIO_WritePin(pdata->enable_gpio_bank, pdata->enable_gpio_pin, GPIO_PIN_RESET);

    pbdrv_sound_hdma.Instance = pdata->dma;
    pbdrv_sound_hdma.Init.Channel = pdata->dma_ch;
    pbdrv_sound_hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    pbdrv_sound_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    pbdrv_sound_hdma.Init.MemInc = DMA_MINC_ENABLE;
    pbdrv_sound_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    pbdrv_sound_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    pbdrv_sound_hdma.Init.Mode = DMA_CIRCULAR;
    pbdrv_sound_hdma.Init.Priority = DMA_PRIORITY_HIGH;
    pbdrv_sound_hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    pbdrv_sound_hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    pbdrv_sound_hdma.Init.MemBurst = DMA_MBURST_SINGLE;
    pbdrv_sound_hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&pbdrv_sound_hdma);

    pbdrv_sound_hdac.Instance = pdata->dac;
    HAL_DAC_Init(&pbdrv_sound_hdac);

    __HAL_LINKDMA(&pbdrv_sound_hdac, DMA_Handle1, pbdrv_sound_hdma);

    DAC_ChannelConfTypeDef channel_config;
    channel_config.DAC_Trigger = pdata->dac_trigger;
    channel_config.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&pbdrv_sound_hdac, &channel_config, pdata->dac_ch);

    pbdrv_sound_htim.Instance = pdata->tim;
    pbdrv_sound_htim.Init.Prescaler = 0;
    pbdrv_sound_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    pbdrv_sound_htim.Init.Period = 0xffff;
    pbdrv_sound_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&pbdrv_sound_htim);

    TIM_MasterConfigTypeDef master_config;
    master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&pbdrv_sound_htim, &master_config);

    HAL_TIM_Base_Start(&pbdrv_sound_htim);

    HAL_NVIC_SetPriority(pdata->dma_irq, 4, 0);
    HAL_NVIC_EnableIRQ(pdata->dma_irq);
}

void pbdrv_sound_start(const uint16_t *data, uint32_t length, uint32_t sample_rate) {
    const pbdrv_sound_stm32_hal_dac_platform_data_t *pdata = &pbdrv_sound_stm32_hal_dac_platform_data;

    HAL_GPIO_WritePin(pdata->enable_gpio_bank, pdata->enable_gpio_pin, GPIO_PIN_SET);
    pbdrv_sound_htim.Init.Period = pdata->tim_clock_rate / sample_rate - 1;
    HAL_TIM_Base_Init(&pbdrv_sound_htim);
    HAL_DAC_Start_DMA(&pbdrv_sound_hdac, pdata->dac_ch, (uint32_t *)data, length, DAC_ALIGN_12B_L);
}

void pbdrv_sound_stop(void) {
    const pbdrv_sound_stm32_hal_dac_platform_data_t *pdata = &pbdrv_sound_stm32_hal_dac_platform_data;

    HAL_GPIO_WritePin(pdata->enable_gpio_bank, pdata->enable_gpio_pin, GPIO_PIN_RESET);
    HAL_DAC_Stop_DMA(&pbdrv_sound_hdac, pdata->dac_ch);
}

void pbdrv_sound_stm32_hal_dac_handle_dma_irq(void) {
    HAL_DMA_IRQHandler(&pbdrv_sound_hdma);
}

#endif // PBDRV_CONFIG_SOUND_STM32_HAL_DAC
