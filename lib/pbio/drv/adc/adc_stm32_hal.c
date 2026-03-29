// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2026 The Pybricks Authors

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

#include <pbdrv/adc.h>

#include STM32_HAL_H

#if defined(STM32H5)
#include <stm32h5xx_ll_dma.h>
#endif

#define PBDRV_ADC_PERIOD_MS 10  // polling period in milliseconds

static TIM_HandleTypeDef pbdrv_adc_htim;
static DMA_HandleTypeDef pbdrv_adc_hdma;
static ADC_HandleTypeDef pbdrv_adc_hadc;

#if defined(STM32H5)
static DMA_NodeTypeDef pbdrv_adc_dma_node;
static DMA_QListTypeDef pbdrv_adc_dma_queue;
#endif

static uint32_t pbdrv_adc_dma_buffer[PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS];
static uint32_t pbdrv_adc_error_count;
static uint32_t pbdrv_adc_last_error;

pbio_error_t pbdrv_adc_await_new_samples(pbio_os_state_t *state, uint32_t *start_time_us, uint32_t future_us) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    if (ch >= PBDRV_CONFIG_ADC_STM32_HAL_ADC_NUM_CHANNELS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *value = pbdrv_adc_dma_buffer[ch];

    return PBIO_SUCCESS;
}

void pbdrv_adc_stm32_hal_handle_irq(void) {
    HAL_DMA_IRQHandler(&pbdrv_adc_hdma);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    // not used
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    pbdrv_adc_error_count++;
    pbdrv_adc_last_error = hadc->ErrorCode;
}


void pbdrv_adc_init(void) {

    // Timer to trigger ADC

    pbdrv_adc_htim.Instance = PBDRV_CONFIG_ADC_STM32_HAL_TIMER_INSTANCE;
    pbdrv_adc_htim.Init.Prescaler = SystemCoreClock / 1000000 - 1; // should give 1kHz clock
    pbdrv_adc_htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    pbdrv_adc_htim.Init.Period = PBDRV_ADC_PERIOD_MS * 1000 - 1;
    pbdrv_adc_htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    HAL_TIM_Base_Init(&pbdrv_adc_htim);

    TIM_MasterConfigTypeDef tim_master_config;
    tim_master_config.MasterOutputTrigger = TIM_TRGO_UPDATE;
    #ifdef STM32L4
    tim_master_config.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    #endif
    tim_master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&pbdrv_adc_htim, &tim_master_config);

    // using DMA

    pbdrv_adc_hadc.Instance = PBDRV_CONFIG_ADC_STM32_HAL_ADC_INSTANCE;

    pbdrv_adc_hdma.Instance = PBDRV_CONFIG_ADC_STM32_HAL_DMA_INSTANCE;
    #if defined(STM32L4) || defined(STM32H5)
    pbdrv_adc_hdma.Init.Request = PBDRV_CONFIG_ADC_STM32_HAL_DMA_REQUEST;
    #else
    pbdrv_adc_hdma.Init.Channel = PBDRV_CONFIG_ADC_STM32_HAL_DMA_CHANNEL;
    #endif
    #if defined(STM32H5)
    pbdrv_adc_hdma.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    pbdrv_adc_hdma.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    pbdrv_adc_hdma.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    pbdrv_adc_hdma.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    pbdrv_adc_hdma.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;

    if (HAL_DMAEx_List_Init(&pbdrv_adc_hdma) != HAL_OK) {
        pbdrv_adc_error_count++;
        pbdrv_adc_last_error = pbdrv_adc_hdma.ErrorCode;
        return;
    }

    DMA_NodeConfTypeDef dma_node_config = { };
    dma_node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_config.Init.Request = PBDRV_CONFIG_ADC_STM32_HAL_DMA_REQUEST;
    dma_node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_config.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_node_config.Init.SrcInc = DMA_SINC_FIXED;
    dma_node_config.Init.DestInc = DMA_DINC_INCREMENTED;
    dma_node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
    dma_node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    dma_node_config.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    dma_node_config.Init.SrcBurstLength = 1;
    dma_node_config.Init.DestBurstLength = 1;
    dma_node_config.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    dma_node_config.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_config.Init.Mode = DMA_NORMAL;
    dma_node_config.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_config.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    dma_node_config.TriggerConfig.TriggerMode = DMA_TRIGM_BLOCK_TRANSFER;
    dma_node_config.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_config.TriggerConfig.TriggerSelection = 0;
    dma_node_config.SrcAddress = (uint32_t)&pbdrv_adc_hadc.Instance->DR;
    dma_node_config.DstAddress = (uint32_t)pbdrv_adc_dma_buffer;
    dma_node_config.DataSize = sizeof(pbdrv_adc_dma_buffer);

    if (HAL_DMAEx_List_BuildNode(&dma_node_config, &pbdrv_adc_dma_node) != HAL_OK) {
        pbdrv_adc_error_count++;
        pbdrv_adc_last_error = pbdrv_adc_hdma.ErrorCode;
        return;
    }

    if (HAL_DMAEx_List_InsertNode_Tail(&pbdrv_adc_dma_queue, &pbdrv_adc_dma_node) != HAL_OK) {
        pbdrv_adc_error_count++;
        pbdrv_adc_last_error = pbdrv_adc_dma_queue.ErrorCode;
        return;
    }

    if (HAL_DMAEx_List_SetCircularMode(&pbdrv_adc_dma_queue) != HAL_OK) {
        pbdrv_adc_error_count++;
        pbdrv_adc_last_error = pbdrv_adc_dma_queue.ErrorCode;
        return;
    }

    if (HAL_DMAEx_List_LinkQ(&pbdrv_adc_hdma, &pbdrv_adc_dma_queue) != HAL_OK) {
        pbdrv_adc_error_count++;
        pbdrv_adc_last_error = pbdrv_adc_hdma.ErrorCode;
        return;
    }
    #else
    pbdrv_adc_hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    pbdrv_adc_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    pbdrv_adc_hdma.Init.MemInc = DMA_MINC_ENABLE;
    pbdrv_adc_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    pbdrv_adc_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    pbdrv_adc_hdma.Init.Mode = DMA_CIRCULAR;
    pbdrv_adc_hdma.Init.Priority = DMA_PRIORITY_MEDIUM;
    HAL_DMA_Init(&pbdrv_adc_hdma);
    #endif

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

    #if defined(STM32L4) || defined(STM32H5)
    HAL_ADCEx_Calibration_Start(&pbdrv_adc_hadc, ADC_SINGLE_ENDED);
    #endif

    __HAL_LINKDMA(&pbdrv_adc_hadc, DMA_Handle, pbdrv_adc_hdma);
    HAL_NVIC_SetPriority(PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ, 7, 0);
    HAL_NVIC_EnableIRQ(PBDRV_CONFIG_ADC_STM32_HAL_DMA_IRQ);
    HAL_ADC_Start_DMA(&pbdrv_adc_hadc, pbdrv_adc_dma_buffer, PBIO_ARRAY_SIZE(pbdrv_adc_dma_buffer));
    HAL_TIM_Base_Start(&pbdrv_adc_htim);
}

#endif // PBDRV_CONFIG_ADC_STM32_HAL
