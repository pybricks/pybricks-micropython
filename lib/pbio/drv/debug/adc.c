// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <stdbool.h>
#include <stdint.h>

#include <pbio/error.h>
#include "sys/process.h"

#define USE_HAL_DRIVER
#include "stm32f4xx.h"

#define PBDRV_ADC_PERIOD_MS 10  // polling period in milliseconds
#define PBDRV_NUM_ADC_CH 6      // number of channels

static TIM_HandleTypeDef pbdrv_adc_htim;
static DMA_HandleTypeDef pbdrv_adc_hdma;
static ADC_HandleTypeDef pbdrv_adc_hadc;

static uint32_t pbdrv_adc_dma_buffer[PBDRV_NUM_ADC_CH];

PROCESS(pbdrv_adc_process, "ADC");

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    if (ch >= PBDRV_NUM_ADC_CH) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *value = pbdrv_adc_dma_buffer[ch];

    return PBIO_SUCCESS;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc) {
    TIM_MasterConfigTypeDef tim_master_config;
    GPIO_InitTypeDef gpio_init;
    ADC_ChannelConfTypeDef adc_ch_config;

    // clocks are enabled in sys.c
    assert_param(__HAL_RCC_TIM2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_DMA2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_ADC3_IS_CLK_ENABLED());

    pbdrv_adc_htim.Instance = TIM2;
    pbdrv_adc_htim.Init.Prescaler = SystemCoreClock / 1000000 - 1; // should give 1kHz clock
    pbdrv_adc_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    pbdrv_adc_htim.Init.Period = PBDRV_ADC_PERIOD_MS * 1000 - 1;
    pbdrv_adc_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_Base_Init(&pbdrv_adc_htim);

    tim_master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    tim_master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&pbdrv_adc_htim, &tim_master_config);

    // PA3, A0

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_3;
    adc_ch_config.Rank = 1;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC0, A1

    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_10;
    adc_ch_config.Rank = 2;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC3, A2

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_13;
    adc_ch_config.Rank = 3;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PF3, A3

    gpio_init.Pin = GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_9;
    adc_ch_config.Rank = 4;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PF5, A4

    gpio_init.Pin = GPIO_PIN_5;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_15;
    adc_ch_config.Rank = 5;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PF10, A5

    gpio_init.Pin = GPIO_PIN_10;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_8;
    adc_ch_config.Rank = 6;
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    adc_ch_config.Offset = 0;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // using DMA

    pbdrv_adc_hdma.Instance = DMA2_Stream0;
    pbdrv_adc_hdma.Init.Channel = DMA_CHANNEL_2;
    pbdrv_adc_hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    pbdrv_adc_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    pbdrv_adc_hdma.Init.MemInc = DMA_MINC_ENABLE;
    pbdrv_adc_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    pbdrv_adc_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    pbdrv_adc_hdma.Init.Mode = DMA_CIRCULAR;
    pbdrv_adc_hdma.Init.Priority = DMA_PRIORITY_MEDIUM;
    pbdrv_adc_hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    pbdrv_adc_hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    pbdrv_adc_hdma.Init.MemBurst = DMA_MBURST_SINGLE;
    pbdrv_adc_hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    __HAL_LINKDMA(hadc, DMA_Handle, pbdrv_adc_hdma);

    HAL_DMA_Init(&pbdrv_adc_hdma);

    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

void DMA2_Stream0_IRQHandler() {
    HAL_DMA_IRQHandler(&pbdrv_adc_hdma);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    process_poll(&pbdrv_adc_process);
}

static void pbdrv_adc_poll() {
    // TODO: filter incoming analog values
}

static void pbdrv_adc_exit() {
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

    pbdrv_adc_hadc.Instance = ADC3;
    pbdrv_adc_hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    pbdrv_adc_hadc.Init.Resolution = ADC_RESOLUTION_12B;
    pbdrv_adc_hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    pbdrv_adc_hadc.Init.ScanConvMode = ENABLE;
    pbdrv_adc_hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    pbdrv_adc_hadc.Init.ContinuousConvMode = DISABLE;
    pbdrv_adc_hadc.Init.NbrOfConversion = PBDRV_NUM_ADC_CH;
    pbdrv_adc_hadc.Init.DiscontinuousConvMode = DISABLE;
    pbdrv_adc_hadc.Init.NbrOfDiscConversion = 0;
    pbdrv_adc_hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
    pbdrv_adc_hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    pbdrv_adc_hadc.Init.DMAContinuousRequests = ENABLE;

    HAL_ADC_Init(&pbdrv_adc_hadc);
    HAL_ADC_Start_DMA(&pbdrv_adc_hadc, pbdrv_adc_dma_buffer, PBDRV_NUM_ADC_CH);
    HAL_TIM_Base_Start(&pbdrv_adc_htim);

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}
