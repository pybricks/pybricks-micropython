// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// UART driver for STM32F4x using IRQ.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_STM32F4_LL_IRQ

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <contiki.h>
#include <contiki-lib.h>

#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_usart.h>

#include <pbdrv/uart.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "./uart_stm32f4_ll_irq.h"
#include "../../src/processes.h"

#define RX_DATA_SIZE 64 // must be power of 2 for ring buffer!

typedef struct {
    /** Public UART device handle. */
    pbdrv_uart_dev_t uart_dev;
    /** Platform-specific data */
    const pbdrv_uart_stm32f4_ll_irq_platform_data_t *pdata;
    /** Circular buffer for caching received bytes. */
    struct ringbuf rx_buf;
    /** Timer for read timeout. */
    struct etimer read_timer;
    /** Timer for write timeout. */
    struct etimer write_timer;
    /** The buffer passed to the read_begin function. */
    uint8_t *read_buf;
    /** The length of read_buf in bytes. */
    uint8_t read_length;
    /** The current position in read_buf. */
    uint8_t read_pos;
    /** The buffer passed to the write_begin function. */
    uint8_t *write_buf;
    /** The length of write_buf in bytes. */
    uint8_t write_length;
    /** The current position in write_buf. */
    volatile uint8_t write_pos;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART];
static uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART][RX_DATA_SIZE];

PROCESS(pbdrv_uart_process, "UART");

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (!pbdrv_uart[id].pdata) {
        // has not been initalized yet
        return PBIO_ERROR_AGAIN;
    }

    *uart_dev = &pbdrv_uart[id].uart_dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    if (uart->read_buf) {
        // Another read operation is already in progress.
        return PBIO_ERROR_AGAIN;
    }

    uart->read_buf = msg;
    uart->read_length = length;
    uart->read_pos = 0;

    etimer_set(&uart->read_timer, timeout);

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    // If read_pos is less that read_length then we have not read everything yet
    if (uart->read_pos < uart->read_length) {
        if (etimer_expired(&uart->read_timer)) {
            uart->read_buf = NULL;
            return PBIO_ERROR_TIMEDOUT;
        }
        return PBIO_ERROR_AGAIN;
    }

    etimer_stop(&uart->read_timer);

    return PBIO_SUCCESS;
}

void pbdrv_uart_read_cancel(pbdrv_uart_dev_t *uart_dev) {
    // TODO
}

pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    if (uart->write_buf) {
        // Another write operation is already in progress.
        return PBIO_ERROR_AGAIN;
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    etimer_set(&uart->write_timer, timeout);

    LL_USART_EnableIT_TXE(uart->pdata->uart);

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    // If write_pos is less that write_length then we have not written everything yet.
    if (uart->write_pos < uart->write_length) {
        if (etimer_expired(&uart->write_timer)) {
            LL_USART_DisableIT_TXE(uart->pdata->uart);
            LL_USART_DisableIT_TC(uart->pdata->uart);
            uart->write_buf = NULL;
            return PBIO_ERROR_TIMEDOUT;
        }
        return PBIO_ERROR_AGAIN;
    }

    etimer_stop(&uart->write_timer);

    return PBIO_SUCCESS;
}

void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart_dev) {
    // TODO
}

pbio_error_t pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    USART_TypeDef *USARTx = uart->pdata->uart;
    uint32_t periphclk = LL_RCC_PERIPH_FREQUENCY_NO;
    LL_RCC_ClocksTypeDef rcc_clocks;

    LL_RCC_GetSystemClocksFreq(&rcc_clocks);
    if (USARTx == USART1
        #if defined(USART6)
        || USARTx == USART6
        #endif
        #if defined(UART9)
        || USARTx == UART9
        #endif
        #if defined(UART10)
        || USARTx == UART10
        #endif
        ) {
        periphclk = rcc_clocks.PCLK2_Frequency;
    } else {
        periphclk = rcc_clocks.PCLK1_Frequency;
    }

    LL_USART_SetBaudRate(USARTx, periphclk, LL_USART_OVERSAMPLING_16, baud);

    return PBIO_SUCCESS;
}

void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev) {
}

void pbdrv_uart_stm32f4_ll_irq_handle_irq(uint8_t id) {
    pbdrv_uart_t *uart = &pbdrv_uart[id];
    USART_TypeDef *USARTx = uart->pdata->uart;
    uint32_t sr = USARTx->SR;

    if (sr & USART_SR_RXNE) {
        ringbuf_put(&uart->rx_buf, LL_USART_ReceiveData8(USARTx));
        process_poll(&pbdrv_uart_process);
    }

    if (sr & USART_SR_ORE) {
        // clears interrupt
        LL_USART_ReceiveData8(USARTx);
    }

    if (USARTx->CR1 & USART_CR1_TXEIE && sr & USART_SR_TXE) {
        LL_USART_TransmitData8(USARTx, uart->write_buf[uart->write_pos++]);
        // When all bytes have been written, wait for the Tx complete interrupt.
        if (uart->write_pos == uart->write_length) {
            LL_USART_DisableIT_TXE(USARTx);
            LL_USART_EnableIT_TC(USARTx);
        }
    }

    if (USARTx->CR1 & USART_CR1_TCIE && sr & USART_SR_TC) {
        LL_USART_DisableIT_TC(USARTx);
        process_poll(&pbdrv_uart_process);
    }
}

static void handle_poll(void) {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART; i++) {
        pbdrv_uart_t *uart = &pbdrv_uart[i];

        // if receive is pending and we have not received all bytes yet
        while (uart->read_buf && uart->read_pos < uart->read_length) {
            int c = ringbuf_get(&uart->rx_buf);
            if (c == -1) {
                break;
            }
            uart->read_buf[uart->read_pos++] = c;
        }

        // broadcast when read_buf is full
        if (uart->read_buf && uart->read_pos == uart->read_length) {
            // clearing read_buf to prevent multiple broadcasts
            uart->read_buf = NULL;
            process_post(PROCESS_BROADCAST, PROCESS_EVENT_COM, NULL);
        }

        // broadcast when write_buf is drained
        if (uart->write_buf && uart->write_pos == uart->write_length) {
            // clearing write_buf to prevent multiple broadcasts
            uart->write_buf = NULL;
            process_post(PROCESS_BROADCAST, PROCESS_EVENT_COM, NULL);
        }
    }
}

static void handle_exit(void) {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART; i++) {
        const pbdrv_uart_stm32f4_ll_irq_platform_data_t *pdata = &pbdrv_uart_stm32f4_ll_irq_platform_data[i];
        LL_USART_Disable(pdata->uart);
        NVIC_DisableIRQ(pdata->irq);
    }
}

PROCESS_THREAD(pbdrv_uart_process, ev, data) {
    PROCESS_POLLHANDLER(handle_poll());
    PROCESS_EXITHANDLER(handle_exit());

    PROCESS_BEGIN();

    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART; i++) {
        const pbdrv_uart_stm32f4_ll_irq_platform_data_t *pdata = &pbdrv_uart_stm32f4_ll_irq_platform_data[i];
        uint8_t *rx_data = pbdrv_uart_rx_data[i];
        pbdrv_uart_t *uart = &pbdrv_uart[i];
        uart->pdata = pdata;
        ringbuf_init(&uart->rx_buf, rx_data, RX_DATA_SIZE);

        // configure UART

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
        LL_USART_EnableIT_RXNE(pdata->uart);

        NVIC_SetPriority(pdata->irq, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
        NVIC_EnableIRQ(pdata->irq);

        // start receiving as soon as everything is configured
        LL_USART_Enable(pdata->uart);
    }

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_UART_STM32F4_LL_IRQ
