// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors
// Copyright (c) 2020 Tilen MAJERLE
// https://github.com/MaJerle/stm32-usart-uart-dma-rx-tx/blob/master/projects/usart_rx_idle_line_irq_rtos_L4_multi_instance/Src/main.c

// UART driver for STM32L4x using DMA.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_STM32L4_LL_DMA

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <contiki.h>

#include <pbdrv/uart.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "./uart_stm32l4_ll_dma.h"
#include "../core.h"

#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_lpuart.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_usart.h"

#define RX_DATA_SIZE 64 // must be power of 2 for ring buffer!

typedef struct {
    pbdrv_uart_dev_t uart_dev;
    const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata;
    struct etimer rx_timer;
    struct etimer tx_timer;
    volatile uint8_t *rx_data;
    uint8_t rx_tail;
    uint8_t *read_buf;
    uint8_t read_length;
    /** Callback to call on read or write completion events */
    pbdrv_uart_poll_callback_t poll_callback;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART];
static volatile uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART][RX_DATA_SIZE];

void pbdrv_uart_set_poll_callback(pbdrv_uart_dev_t *uart_dev, pbdrv_uart_poll_callback_t callback) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    uart->poll_callback = callback;
}

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (!pbdrv_uart[id].pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    *uart_dev = &pbdrv_uart[id].uart_dev;

    return PBIO_SUCCESS;
}

static void volatile_copy(volatile uint8_t *src, uint8_t *dst, uint8_t size) {
    for (int i = 0; i < size; i++) {
        dst[i] = src[i];
    }
}

static void dma_clear_tc(DMA_TypeDef *DMAx, uint32_t channel) {
    switch (channel) {
        case LL_DMA_CHANNEL_1:
            LL_DMA_ClearFlag_TC1(DMAx);
            break;
        case LL_DMA_CHANNEL_2:
            LL_DMA_ClearFlag_TC2(DMAx);
            break;
        case LL_DMA_CHANNEL_3:
            LL_DMA_ClearFlag_TC3(DMAx);
            break;
        case LL_DMA_CHANNEL_4:
            LL_DMA_ClearFlag_TC4(DMAx);
            break;
        case LL_DMA_CHANNEL_5:
            LL_DMA_ClearFlag_TC5(DMAx);
            break;
        case LL_DMA_CHANNEL_6:
            LL_DMA_ClearFlag_TC6(DMAx);
            break;
        case LL_DMA_CHANNEL_7:
            LL_DMA_ClearFlag_TC7(DMAx);
            break;
    }
}

static void dma_clear_ht(DMA_TypeDef *DMAx, uint32_t channel) {
    switch (channel) {
        case LL_DMA_CHANNEL_1:
            LL_DMA_ClearFlag_HT1(DMAx);
            break;
        case LL_DMA_CHANNEL_2:
            LL_DMA_ClearFlag_HT2(DMAx);
            break;
        case LL_DMA_CHANNEL_3:
            LL_DMA_ClearFlag_HT3(DMAx);
            break;
        case LL_DMA_CHANNEL_4:
            LL_DMA_ClearFlag_HT4(DMAx);
            break;
        case LL_DMA_CHANNEL_5:
            LL_DMA_ClearFlag_HT5(DMAx);
            break;
        case LL_DMA_CHANNEL_6:
            LL_DMA_ClearFlag_HT6(DMAx);
            break;
        case LL_DMA_CHANNEL_7:
            LL_DMA_ClearFlag_HT7(DMAx);
            break;
    }
}

static void dma_clear_te(DMA_TypeDef *DMAx, uint32_t channel) {
    switch (channel) {
        case LL_DMA_CHANNEL_1:
            LL_DMA_ClearFlag_TE1(DMAx);
            break;
        case LL_DMA_CHANNEL_2:
            LL_DMA_ClearFlag_TE2(DMAx);
            break;
        case LL_DMA_CHANNEL_3:
            LL_DMA_ClearFlag_TE3(DMAx);
            break;
        case LL_DMA_CHANNEL_4:
            LL_DMA_ClearFlag_TE4(DMAx);
            break;
        case LL_DMA_CHANNEL_5:
            LL_DMA_ClearFlag_TE5(DMAx);
            break;
        case LL_DMA_CHANNEL_6:
            LL_DMA_ClearFlag_TE6(DMAx);
            break;
        case LL_DMA_CHANNEL_7:
            LL_DMA_ClearFlag_TE7(DMAx);
            break;
    }
}

static bool dma_is_tc(DMA_TypeDef *DMAx, uint32_t channel) {
    switch (channel) {
        case LL_DMA_CHANNEL_1:
            return LL_DMA_IsActiveFlag_TC1(DMAx);
        case LL_DMA_CHANNEL_2:
            return LL_DMA_IsActiveFlag_TC2(DMAx);
        case LL_DMA_CHANNEL_3:
            return LL_DMA_IsActiveFlag_TC3(DMAx);
        case LL_DMA_CHANNEL_4:
            return LL_DMA_IsActiveFlag_TC4(DMAx);
        case LL_DMA_CHANNEL_5:
            return LL_DMA_IsActiveFlag_TC5(DMAx);
        case LL_DMA_CHANNEL_6:
            return LL_DMA_IsActiveFlag_TC6(DMAx);
        case LL_DMA_CHANNEL_7:
            return LL_DMA_IsActiveFlag_TC7(DMAx);
        default:
            return false;
    }
}

static bool dma_is_ht(DMA_TypeDef *DMAx, uint32_t channel) {
    switch (channel) {
        case LL_DMA_CHANNEL_1:
            return LL_DMA_IsActiveFlag_HT1(DMAx);
        case LL_DMA_CHANNEL_2:
            return LL_DMA_IsActiveFlag_HT2(DMAx);
        case LL_DMA_CHANNEL_3:
            return LL_DMA_IsActiveFlag_HT3(DMAx);
        case LL_DMA_CHANNEL_4:
            return LL_DMA_IsActiveFlag_HT4(DMAx);
        case LL_DMA_CHANNEL_5:
            return LL_DMA_IsActiveFlag_HT5(DMAx);
        case LL_DMA_CHANNEL_6:
            return LL_DMA_IsActiveFlag_HT6(DMAx);
        case LL_DMA_CHANNEL_7:
            return LL_DMA_IsActiveFlag_HT7(DMAx);
        default:
            return false;
    }
}

static uint32_t pbdrv_uart_get_num_available(pbdrv_uart_t *uart) {
    // Head is the last position that DMA wrote to.
    uint32_t rx_head = RX_DATA_SIZE - LL_DMA_GetDataLength(uart->pdata->rx_dma, uart->pdata->rx_dma_ch);
    return (rx_head - uart->rx_tail) & (RX_DATA_SIZE - 1);
}

PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    PT_BEGIN(pt);

    if (uart->read_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->read_buf = msg;
    uart->read_length = length;

    etimer_set(&uart->rx_timer, timeout);

    // Wait until we have enough data or timeout. If there is enough data
    // already, this completes right away without yielding once first.
    PT_WAIT_UNTIL(pt, pbdrv_uart_get_num_available(uart) >= uart->read_length || etimer_expired(&uart->rx_timer));
    if (etimer_expired(&uart->rx_timer)) {
        uart->read_buf = NULL;
        uart->read_length = 0;
        *err = PBIO_ERROR_TIMEDOUT;
        PT_EXIT(pt);
    }

    // Copy from ring buffer to user buffer, taking care of wrap-around.
    if (uart->rx_tail + uart->read_length > RX_DATA_SIZE) {
        uint32_t partial_size = RX_DATA_SIZE - uart->rx_tail;
        volatile_copy(&uart->rx_data[uart->rx_tail], &uart->read_buf[0], partial_size);
        volatile_copy(&uart->rx_data[0], &uart->read_buf[partial_size], uart->read_length - partial_size);
    } else {
        volatile_copy(&uart->rx_data[uart->rx_tail], &uart->read_buf[0], uart->read_length);
    }

    uart->rx_tail = (uart->rx_tail + uart->read_length) & (RX_DATA_SIZE - 1);
    uart->read_buf = NULL;
    uart->read_length = 0;

    etimer_stop(&uart->rx_timer);
    *err = PBIO_SUCCESS;

    PT_END(pt);
}

PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata = uart->pdata;

    PT_BEGIN(pt);

    if (LL_USART_IsEnabledDMAReq_TX(pdata->uart)) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    LL_DMA_DisableChannel(pdata->tx_dma, pdata->tx_dma_ch);
    LL_DMA_SetMemoryAddress(pdata->tx_dma, pdata->tx_dma_ch, (uint32_t)msg);
    LL_DMA_SetDataLength(pdata->tx_dma, pdata->tx_dma_ch, length);
    dma_clear_tc(pdata->tx_dma, pdata->tx_dma_ch);
    dma_clear_ht(pdata->tx_dma, pdata->tx_dma_ch);
    dma_clear_te(pdata->tx_dma, pdata->tx_dma_ch);
    LL_DMA_EnableChannel(pdata->tx_dma, pdata->tx_dma_ch);
    LL_USART_ClearFlag_TC(pdata->uart);
    LL_USART_EnableDMAReq_TX(pdata->uart);

    etimer_set(&uart->tx_timer, timeout);

    PT_WAIT_WHILE(pt, LL_USART_IsEnabledDMAReq_TX(pdata->uart) && !etimer_expired(&uart->tx_timer));
    if (etimer_expired(&uart->tx_timer)) {
        LL_USART_DisableDMAReq_TX(pdata->uart);
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->tx_timer);
        *err = PBIO_SUCCESS;
    }

    PT_END(pt);
}

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    USART_TypeDef *USARTx = uart->pdata->uart;
    uint32_t periphclk = LL_RCC_PERIPH_FREQUENCY_NO;

    if (USARTx == LPUART1) {
        periphclk = LL_RCC_GetLPUARTClockFreq(LL_RCC_LPUART1_CLKSOURCE);
        LL_LPUART_SetBaudRate(USARTx, periphclk, baud);
    } else {
        if (USARTx == USART1) {
            periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART1_CLKSOURCE);
        } else if (USARTx == USART2) {
            periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART2_CLKSOURCE);
        }
        #if defined(USART3)
        else if (USARTx == USART3) {
            periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART3_CLKSOURCE);
        }
        #endif /* USART3 */
        #if defined(UART4)
        else if (USARTx == UART4) {
            periphclk = LL_RCC_GetUARTClockFreq(LL_RCC_UART4_CLKSOURCE);
        }
        #endif /* UART4 */
        #if defined(UART5)
        else if (USARTx == UART5) {
            periphclk = LL_RCC_GetUARTClockFreq(LL_RCC_UART5_CLKSOURCE);
        }
        #endif /* UART5 */
        else {
            return;
        }

        LL_USART_SetBaudRate(USARTx, periphclk, LL_USART_OVERSAMPLING_16, baud);
    }
}

void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    uart->read_buf = NULL;
    uart->read_length = 0;
    // Clears the ring buffer by setting tail equal to head.
    uart->rx_tail = RX_DATA_SIZE - LL_DMA_GetDataLength(uart->pdata->rx_dma, uart->pdata->rx_dma_ch);
}

static void poll_process_by_id(uint8_t id) {
    pbdrv_uart_t *uart = &pbdrv_uart[id];
    if (uart->poll_callback) {
        uart->poll_callback(&uart->uart_dev);
    }
}

void pbdrv_uart_stm32l4_ll_dma_handle_tx_dma_irq(uint8_t id) {
    const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata = &pbdrv_uart_stm32l4_ll_dma_platform_data[id];
    if (LL_DMA_IsEnabledIT_TC(pdata->tx_dma, pdata->tx_dma_ch) && dma_is_tc(pdata->tx_dma, pdata->tx_dma_ch)) {
        dma_clear_tc(pdata->tx_dma, pdata->tx_dma_ch);
        LL_USART_DisableDMAReq_TX(pdata->uart);
        poll_process_by_id(id);
    }
}

void pbdrv_uart_stm32l4_ll_dma_handle_rx_dma_irq(uint8_t id) {
    const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata = &pbdrv_uart_stm32l4_ll_dma_platform_data[id];

    if (LL_DMA_IsEnabledIT_HT(pdata->rx_dma, pdata->rx_dma_ch) && dma_is_ht(pdata->rx_dma, pdata->rx_dma_ch)) {
        dma_clear_ht(pdata->rx_dma, pdata->rx_dma_ch);
        poll_process_by_id(id);
    }

    if (LL_DMA_IsEnabledIT_TC(pdata->rx_dma, pdata->rx_dma_ch) && dma_is_tc(pdata->rx_dma, pdata->rx_dma_ch)) {
        dma_clear_tc(pdata->rx_dma, pdata->rx_dma_ch);
        poll_process_by_id(id);
    }
}

void pbdrv_uart_stm32l4_ll_dma_handle_uart_irq(uint8_t id) {
    const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata = &pbdrv_uart_stm32l4_ll_dma_platform_data[id];

    if (LL_USART_IsEnabledIT_TC(pdata->uart) && LL_USART_IsActiveFlag_TC(pdata->uart)) {
        LL_USART_DisableIT_TC(pdata->uart);
        LL_USART_ClearFlag_TC(pdata->uart);
        poll_process_by_id(id);
    }

    if (LL_USART_IsEnabledIT_IDLE(pdata->uart) && LL_USART_IsActiveFlag_IDLE(pdata->uart)) {
        LL_USART_ClearFlag_IDLE(pdata->uart);
        poll_process_by_id(id);
    }
}

// Currently not used
void handle_exit(void) {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART; i++) {
        const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata = &pbdrv_uart_stm32l4_ll_dma_platform_data[i];
        LL_USART_Disable(pdata->uart);
        LL_DMA_DisableChannel(pdata->rx_dma, pdata->rx_dma_ch);
        LL_DMA_DisableChannel(pdata->tx_dma, pdata->tx_dma_ch);
        NVIC_DisableIRQ(pdata->uart_irq);
        NVIC_DisableIRQ(pdata->rx_dma_irq);
        NVIC_DisableIRQ(pdata->tx_dma_irq);
    }
}

void pbdrv_uart_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32L4_LL_DMA_NUM_UART; i++) {
        const pbdrv_uart_stm32l4_ll_dma_platform_data_t *pdata = &pbdrv_uart_stm32l4_ll_dma_platform_data[i];
        volatile uint8_t *rx_data = pbdrv_uart_rx_data[i];
        pbdrv_uart_t *uart = &pbdrv_uart[i];
        uart->pdata = pdata;
        uart->rx_data = rx_data;

        // Configure Tx DMA

        LL_DMA_SetPeriphRequest(pdata->tx_dma, pdata->tx_dma_ch, pdata->tx_dma_req);
        LL_DMA_SetDataTransferDirection(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
        LL_DMA_SetChannelPriorityLevel(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetMode(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_MODE_NORMAL);
        LL_DMA_SetPeriphIncMode(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(pdata->tx_dma, pdata->tx_dma_ch, LL_DMA_MDATAALIGN_BYTE);
        LL_DMA_SetPeriphAddress(pdata->tx_dma, pdata->tx_dma_ch, (uint32_t)&pdata->uart->TDR);

        LL_DMA_EnableIT_TC(pdata->tx_dma, pdata->tx_dma_ch);

        NVIC_SetPriority(pdata->tx_dma_irq, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 1));
        NVIC_EnableIRQ(pdata->tx_dma_irq);

        // Configure Rx DMA

        LL_DMA_SetPeriphRequest(pdata->rx_dma, pdata->rx_dma_ch, pdata->rx_dma_req);
        LL_DMA_SetDataTransferDirection(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
        LL_DMA_SetChannelPriorityLevel(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_PRIORITY_LOW);
        LL_DMA_SetMode(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_MODE_CIRCULAR);
        LL_DMA_SetPeriphIncMode(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_PERIPH_NOINCREMENT);
        LL_DMA_SetMemoryIncMode(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_MEMORY_INCREMENT);
        LL_DMA_SetPeriphSize(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_PDATAALIGN_BYTE);
        LL_DMA_SetMemorySize(pdata->rx_dma, pdata->rx_dma_ch, LL_DMA_MDATAALIGN_BYTE);
        LL_DMA_SetPeriphAddress(pdata->rx_dma, pdata->rx_dma_ch, (uint32_t)&pdata->uart->RDR);
        LL_DMA_SetMemoryAddress(pdata->rx_dma, pdata->rx_dma_ch, (uint32_t)rx_data);
        LL_DMA_SetDataLength(pdata->rx_dma, pdata->rx_dma_ch, RX_DATA_SIZE);

        LL_DMA_EnableIT_HT(pdata->rx_dma, pdata->rx_dma_ch);
        LL_DMA_EnableIT_TC(pdata->rx_dma, pdata->rx_dma_ch);

        NVIC_SetPriority(pdata->rx_dma_irq, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0));
        NVIC_EnableIRQ(pdata->rx_dma_irq);

        // configure UART

        if (pdata->uart == LPUART1) {
            LL_LPUART_InitTypeDef uart_init = {0};
            uart_init.BaudRate = 115200;
            uart_init.DataWidth = LL_LPUART_DATAWIDTH_8B;
            uart_init.StopBits = LL_LPUART_STOPBITS_1;
            uart_init.Parity = LL_LPUART_PARITY_NONE;
            uart_init.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
            uart_init.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
            LL_LPUART_Init(pdata->uart, &uart_init);
            LL_LPUART_EnableDMAReq_RX(pdata->uart);
            LL_LPUART_EnableIT_IDLE(pdata->uart);
        } else {
            LL_USART_InitTypeDef uart_init = {0};
            uart_init.BaudRate = 115200;
            uart_init.DataWidth = LL_USART_DATAWIDTH_8B;
            uart_init.StopBits = LL_USART_STOPBITS_1;
            uart_init.Parity = LL_USART_PARITY_NONE;
            uart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
            uart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
            uart_init.OverSampling = LL_USART_OVERSAMPLING_16;
            LL_USART_Init(pdata->uart, &uart_init);
            LL_USART_ConfigAsyncMode(pdata->uart);
            LL_USART_EnableDMAReq_RX(pdata->uart);
            LL_USART_EnableIT_IDLE(pdata->uart);
        }

        NVIC_SetPriority(pdata->uart_irq, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
        NVIC_EnableIRQ(pdata->uart_irq);

        // start receiving as soon as everything is configured

        LL_DMA_EnableChannel(pdata->rx_dma, pdata->rx_dma_ch);
        LL_USART_Enable(pdata->uart);
    }
}

#endif // PBDRV_CONFIG_UART_STM32L4_LL_DMA
