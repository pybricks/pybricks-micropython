// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// Platform should override HAL_ADC_MspInit() to configure pin mux and ADC
// channel mapping. Example:
//
// GPIO_InitTypeDef gpio_init;
// gpio_init.Pin = GPIO_PIN_3;
// gpio_init.Mode = GPIO_MODE_ANALOG;
// gpio_init.Pull = GPIO_NOPULL;
// gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
// HAL_GPIO_Init(GPIOA, &gpio_init);

// ADC_ChannelConfTypeDef adc_ch_config;
// adc_ch_config.Channel = ADC_CHANNEL_3;
// adc_ch_config.Rank = 1;
// adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
// adc_ch_config.Offset = 0;
// HAL_ADC_ConfigChannel(hadc, &adc_ch_config);


#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_STM32_HAL

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>
#include <pbio/util.h>
#include "sys/process.h"

#include STM32_HAL_H

#define PBDRV_ADC_PERIOD_MS 10  // polling period in milliseconds

static TIM_HandleTypeDef pbdrv_adc_htim;
static DMA_HandleTypeDef pbdrv_adc_hdma;
static ADC_HandleTypeDef pbdrv_adc_hadc;

static uint32_t pbdrv_adc_dma_buffer[PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS];
static uint32_t pbdrv_adc_error_count;
static uint32_t pbdrv_adc_last_error;

PROCESS(pbdrv_adc_process, "ADC");

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    if (ch >= PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *value = pbdrv_adc_dma_buffer[ch];

    return PBIO_SUCCESS;
}

void pbdrv_adc_stm32_hal_handle_irq() {
    HAL_DMA_IRQHandler(&pbdrv_adc_hdma);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    process_poll(&pbdrv_adc_process);
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    pbdrv_adc_error_count++;
    pbdrv_adc_last_error = hadc->ErrorCode;
}

static void pbdrv_adc_poll() {
    // TODO: filter incoming analog values
}

static void pbdrv_adc_exit() {
    HAL_NVIC_DisableIRQ(PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ);
    HAL_TIM_Base_Stop(&pbdrv_adc_htim);
    HAL_TIM_Base_DeInit(&pbdrv_adc_htim);
    HAL_ADC_Stop_DMA(&pbdrv_adc_hadc);
    HAL_ADC_DeInit(&pbdrv_adc_hadc);
    HAL_DMA_DeInit(&pbdrv_adc_hdma);
}

PROCESS_THREAD(pbdrv_adc_process, ev, data) {
    PROCESS_POLLHANDLER(pbdrv_adc_poll());
    PROCESS_EXITHANDLER(pbdrv_adc_exit());

    PROCESS_BEGIN();

    // Timer to trigger ADC

    pbdrv_adc_htim.Instance = PBDRV_CONFIG_ADC_STM32_HAL_TIMER_INSTANCE;
    pbdrv_adc_htim.Init.Prescaler = SystemCoreClock / 1000000 - 1; // should give 1kHz clock
    pbdrv_adc_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    pbdrv_adc_htim.Init.Period = PBDRV_ADC_PERIOD_MS * 1000 - 1;
    pbdrv_adc_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_Base_Init(&pbdrv_adc_htim);

    TIM_MasterConfigTypeDef tim_master_config;
    tim_master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    tim_master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&pbdrv_adc_htim, &tim_master_config);

    // using DMA

    pbdrv_adc_hdma.Instance = PBDRV_CONFIG_ADC_STM32_HAL_DMA_INSTANCE;
#ifdef STM32L4
    pbdrv_adc_hdma.Init.Request = PBDRV_CONFIG_ADC_STM32_HAL_DMA_REQUEST;
#else
    pbdrv_adc_hdma.Init.Channel = PBDRV_CONFIG_ADC_STM32_HAL_DMA_CHANNEL;
#endif
    pbdrv_adc_hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    pbdrv_adc_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    pbdrv_adc_hdma.Init.MemInc = DMA_MINC_ENABLE;
    pbdrv_adc_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    pbdrv_adc_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    pbdrv_adc_hdma.Init.Mode = DMA_CIRCULAR;
    pbdrv_adc_hdma.Init.Priority = DMA_PRIORITY_MEDIUM;

    HAL_DMA_Init(&pbdrv_adc_hdma);

    pbdrv_adc_hadc.Instance = PBDRV_CONFIG_ADC_STM32_HAL_ADC_INSTANCE;
    pbdrv_adc_hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    pbdrv_adc_hadc.Init.Resolution = ADC_RESOLUTION_12B;
    pbdrv_adc_hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    pbdrv_adc_hadc.Init.ScanConvMode = ENABLE;
    pbdrv_adc_hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    pbdrv_adc_hadc.Init.ContinuousConvMode = DISABLE;
    pbdrv_adc_hadc.Init.NbrOfConversion = PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS;
    pbdrv_adc_hadc.Init.DiscontinuousConvMode = DISABLE;
    pbdrv_adc_hadc.Init.NbrOfDiscConversion = 0;
    pbdrv_adc_hadc.Init.ExternalTrigConv = PBDRV_CONFIG_ADC_STM32_HAL_TIMER_TRIGGER;
    pbdrv_adc_hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    pbdrv_adc_hadc.Init.DMAContinuousRequests = ENABLE;

    HAL_ADC_Init(&pbdrv_adc_hadc);

    __HAL_LINKDMA(&pbdrv_adc_hadc, DMA_Handle, pbdrv_adc_hdma);
    HAL_NVIC_SetPriority(PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ, 0, 0);
    HAL_NVIC_EnableIRQ(PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ);
    HAL_ADC_Start_DMA(&pbdrv_adc_hadc, pbdrv_adc_dma_buffer, PBIO_ARRAY_SIZE(pbdrv_adc_dma_buffer));
    HAL_TIM_Base_Start(&pbdrv_adc_htim);

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_ADC_STM32_HAL
