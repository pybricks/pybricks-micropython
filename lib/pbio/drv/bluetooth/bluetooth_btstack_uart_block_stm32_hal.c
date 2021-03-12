// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// STM32 HAL UART driver for BlueKitchen BTStack.

// IMPORTANT: This driver requires a patched STM32 HAL to fix some data loss
// issues. See https://github.com/micropython/stm32lib/pull/12.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_UART

#include <btstack.h>
#undef UNUSED // btstack and stm32 both define UNUSED
#include <stm32f4xx_hal.h>
#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_usart.h>

#include "bluetooth_btstack_run_loop_contiki.h"
#include "bluetooth_btstack_uart_block_stm32_hal.h"

static UART_HandleTypeDef btstack_huart;
static DMA_HandleTypeDef btstack_rx_hdma;
static DMA_HandleTypeDef btstack_tx_hdma;

// uart config
static const btstack_uart_config_t *uart_config;

// data source for integration with BTstack Runloop
static btstack_data_source_t transport_data_source;

static volatile bool send_complete;
static volatile bool receive_complete;

// callbacks
static void (*block_sent)(void);
static void (*block_received)(void);

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    send_complete = true;
    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    receive_complete = true;
    pbdrv_bluetooth_btstack_run_loop_contiki_trigger();
}

static int btstack_uart_block_stm32_hal_init(const btstack_uart_config_t *config) {
    const pbdrv_bluetooth_btstack_uart_block_stm32_platform_data_t *pdata =
        &pbdrv_bluetooth_btstack_uart_block_stm32_platform_data;

    uart_config = config;

    btstack_tx_hdma.Instance = pdata->tx_dma;
    btstack_tx_hdma.Init.Channel = pdata->tx_dma_ch;
    btstack_tx_hdma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    btstack_tx_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    btstack_tx_hdma.Init.MemInc = DMA_MINC_ENABLE;
    btstack_tx_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    btstack_tx_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    btstack_tx_hdma.Init.Mode = DMA_NORMAL;
    btstack_tx_hdma.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    btstack_tx_hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    btstack_tx_hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    btstack_tx_hdma.Init.MemBurst = DMA_MBURST_SINGLE;
    btstack_tx_hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&btstack_tx_hdma);

    btstack_rx_hdma.Instance = pdata->rx_dma;
    btstack_rx_hdma.Init.Channel = pdata->rx_dma_ch;
    btstack_rx_hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    btstack_rx_hdma.Init.PeriphInc = DMA_PINC_DISABLE;
    btstack_rx_hdma.Init.MemInc = DMA_MINC_ENABLE;
    btstack_rx_hdma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    btstack_rx_hdma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    btstack_rx_hdma.Init.Mode = DMA_NORMAL;
    btstack_rx_hdma.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    btstack_rx_hdma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    btstack_rx_hdma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    btstack_rx_hdma.Init.MemBurst = DMA_MBURST_SINGLE;
    btstack_rx_hdma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&btstack_rx_hdma);

    btstack_huart.Instance = pdata->uart;
    btstack_huart.Init.BaudRate = config->baudrate;
    btstack_huart.Init.WordLength = UART_WORDLENGTH_8B;
    btstack_huart.Init.StopBits = UART_STOPBITS_1;
    btstack_huart.Init.Parity = UART_PARITY_NONE;
    btstack_huart.Init.Mode = UART_MODE_TX_RX;
    btstack_huart.Init.HwFlowCtl = config->flowcontrol ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
    btstack_huart.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&btstack_huart);

    __HAL_LINKDMA(&btstack_huart, hdmatx, btstack_tx_hdma);
    __HAL_LINKDMA(&btstack_huart, hdmarx, btstack_rx_hdma);

    HAL_NVIC_SetPriority(pdata->tx_dma_irq, 1, 2);
    HAL_NVIC_EnableIRQ(pdata->tx_dma_irq);
    HAL_NVIC_SetPriority(pdata->rx_dma_irq, 1, 1);
    HAL_NVIC_EnableIRQ(pdata->rx_dma_irq);
    HAL_NVIC_SetPriority(pdata->uart_irq, 1, 0);
    HAL_NVIC_EnableIRQ(pdata->uart_irq);

    return 0;
}

static void btstack_uart_block_stm32_hal_process(btstack_data_source_t *ds, btstack_data_source_callback_type_t callback_type) {
    switch (callback_type) {
        case DATA_SOURCE_CALLBACK_POLL:
            if (send_complete) {
                send_complete = false;
                if (block_sent) {
                    block_sent();
                }
            }
            if (receive_complete) {
                receive_complete = false;
                if (block_received) {
                    block_received();
                }
            }
            break;
        default:
            break;
    }
}

static int btstack_uart_block_stm32_hal_set_baudrate(uint32_t baud) {
    USART_TypeDef *usart = btstack_huart.Instance;
    uint32_t periphclk = LL_RCC_PERIPH_FREQUENCY_NO;
    LL_RCC_ClocksTypeDef rcc_clocks;

    // This assumes STM32F4
    LL_RCC_GetSystemClocksFreq(&rcc_clocks);
    if (usart == USART1
        #if defined(USART6)
        || usart == USART6
        #endif
        #if defined(UART9)
        || usart == UART9
        #endif
        #if defined(UART10)
        || usart == UART10
        #endif
        ) {
        periphclk = rcc_clocks.PCLK2_Frequency;
    } else {
        periphclk = rcc_clocks.PCLK1_Frequency;
    }

    LL_USART_SetBaudRate(usart, periphclk, LL_USART_OVERSAMPLING_16, baud);

    return 0;
}

static int btstack_uart_block_stm32_hal_open(void) {
    btstack_uart_block_stm32_hal_set_baudrate(uart_config->baudrate);

    // set up polling data_source
    btstack_run_loop_set_data_source_handler(&transport_data_source, &btstack_uart_block_stm32_hal_process);
    btstack_run_loop_enable_data_source_callbacks(&transport_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&transport_data_source);

    return 0;
}

static int btstack_uart_block_stm32_hal_close(void) {
    // remove data source
    btstack_run_loop_disable_data_source_callbacks(&transport_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_remove_data_source(&transport_data_source);

    return 0;
}

static void btstack_uart_block_stm32_hal_set_block_received(void (*handler)(void)) {
    block_received = handler;
}

static void btstack_uart_block_stm32_hal_set_block_sent(void (*handler)(void)) {
    block_sent = handler;
}

static int btstack_uart_block_stm32_hal_set_parity(int parity) {
    return 0;
}

static void btstack_uart_block_stm32_hal_receive_block(uint8_t *buffer, uint16_t len) {
    HAL_UART_Receive_DMA(&btstack_huart, buffer, len);
}

static void btstack_uart_block_stm32_hal_send_block(const uint8_t *data, uint16_t size) {
    HAL_UART_Transmit_DMA(&btstack_huart, (uint8_t *)data, size);
}

static const btstack_uart_block_t btstack_uart_block_stm32_hal = {
    .init = btstack_uart_block_stm32_hal_init,
    .open = btstack_uart_block_stm32_hal_open,
    .close = btstack_uart_block_stm32_hal_close,
    .set_block_received = btstack_uart_block_stm32_hal_set_block_received,
    .set_block_sent = btstack_uart_block_stm32_hal_set_block_sent,
    .set_baudrate = btstack_uart_block_stm32_hal_set_baudrate,
    .set_parity = btstack_uart_block_stm32_hal_set_parity,
    .set_flowcontrol = NULL,
    .receive_block = btstack_uart_block_stm32_hal_receive_block,
    .send_block = btstack_uart_block_stm32_hal_send_block,
    .get_supported_sleep_modes = NULL,
    .set_sleep = NULL,
    .set_wakeup_handler = NULL,
};

const btstack_uart_block_t *pbdrv_bluetooth_btstack_uart_block_stm32_hal_instance(void) {
    return &btstack_uart_block_stm32_hal;
}

void pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_tx_dma_irq(void) {
    HAL_DMA_IRQHandler(&btstack_tx_hdma);
}

void pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_rx_dma_irq(void) {
    HAL_DMA_IRQHandler(&btstack_rx_hdma);
}

void pbdrv_bluetooth_btstack_uart_block_stm32_hal_handle_uart_irq(void) {
    HAL_UART_IRQHandler(&btstack_huart);
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_STM32_UART
