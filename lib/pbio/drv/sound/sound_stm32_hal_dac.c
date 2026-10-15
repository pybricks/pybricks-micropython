// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// Sound driver using DAC on STM32 MCU.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_STM32_HAL_DAC

#include <stdint.h>

#include "sound_stm32_hal_dac.h"

#include STM32_HAL_H

static DMA_HandleTypeDef pbdrv_sound_hdma;
static DAC_HandleTypeDef pbdrv_sound_hdac;
static TIM_HandleTypeDef pbdrv_sound_htim;
#if defined(STM32H5)
static DMA_NodeTypeDef pbdrv_sound_dma_node;
static DMA_QListTypeDef pbdrv_sound_dma_queue;
#endif

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
    #if defined(STM32H5)
    // On STM32H5, the GPDMA only repeats a transfer (needed to loop the
    // waveform continuously) when configured in linked-list circular mode.
    // Basic (DMA_NORMAL) mode plays the buffer once, which is only audible as
    // a faint click. So build a single-node queue that links to itself.
    pbdrv_sound_hdma.Init.Request = pdata->dma_req;
    pbdrv_sound_hdma.InitLinkedList.Priority = DMA_HIGH_PRIORITY;
    pbdrv_sound_hdma.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    pbdrv_sound_hdma.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    pbdrv_sound_hdma.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    pbdrv_sound_hdma.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
    HAL_DMAEx_List_Init(&pbdrv_sound_hdma);

    DMA_NodeConfTypeDef dma_node_config = { };
    dma_node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_config.Init.Request = pdata->dma_req;
    dma_node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_config.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_node_config.Init.SrcInc = DMA_SINC_INCREMENTED;
    dma_node_config.Init.DestInc = DMA_DINC_FIXED;
    dma_node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD;
    // The DAC data-holding register must be written with a 32-bit access; a
    // 16-bit (halfword) destination write faults the GPDMA (DTE). The GPDMA
    // zero-extends each 16-bit sample to the 32-bit word.
    dma_node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    dma_node_config.Init.Priority = DMA_HIGH_PRIORITY;
    dma_node_config.Init.SrcBurstLength = 1;
    dma_node_config.Init.DestBurstLength = 1;
    dma_node_config.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_config.Init.Mode = DMA_NORMAL;
    dma_node_config.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_config.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    dma_node_config.TriggerConfig.TriggerMode = DMA_TRIGM_BLOCK_TRANSFER;
    dma_node_config.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_config.TriggerConfig.TriggerSelection = 0;
    // Source/destination/size are placeholders; HAL_DAC_Start_DMA overwrites
    // the head node's registers with the actual waveform buffer and DAC
    // register on each call.
    dma_node_config.SrcAddress = (uint32_t)&pdata->dac->DHR12L1;
    dma_node_config.DstAddress = (uint32_t)&pdata->dac->DHR12L1;
    dma_node_config.DataSize = 2;

    HAL_DMAEx_List_BuildNode(&dma_node_config, &pbdrv_sound_dma_node);
    HAL_DMAEx_List_InsertNode_Tail(&pbdrv_sound_dma_queue, &pbdrv_sound_dma_node);
    HAL_DMAEx_List_SetCircularMode(&pbdrv_sound_dma_queue);
    HAL_DMAEx_List_LinkQ(&pbdrv_sound_hdma, &pbdrv_sound_dma_queue);
    #else
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
    #endif

    pbdrv_sound_hdac.Instance = pdata->dac;
    HAL_DAC_Init(&pbdrv_sound_hdac);

    __HAL_LINKDMA(&pbdrv_sound_hdac, DMA_Handle1, pbdrv_sound_hdma);

    DAC_ChannelConfTypeDef channel_config = { };
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
