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

#include "../core.h"
#include "./uart_stm32f4_ll_irq.h"

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
    /** The buffer of the ongoing async read function. */
    uint8_t *read_buf;
    /** The length of read_buf in bytes. */
    uint8_t read_length;
    /** The current position in read_buf. */
    uint8_t read_pos;
    /** The buffer of the ongoing write function. */
    uint8_t *write_buf;
    /** The length of write_buf in bytes. */
    uint8_t write_length;
    /** The current position in write_buf. */
    volatile uint8_t write_pos;
    /** Callback to call on read or write completion events */
    pbdrv_uart_poll_callback_t poll_callback;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART];
static uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART][RX_DATA_SIZE];

void pbdrv_uart_set_poll_callback(pbdrv_uart_dev_t *uart_dev, pbdrv_uart_poll_callback_t callback) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    uart->poll_callback = callback;
}

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (!pbdrv_uart[id].pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    *uart_dev = &pbdrv_uart[id].uart_dev;

    return PBIO_SUCCESS;
}

PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    // Actual protothread starts here.
    PT_BEGIN(pt);

    if (uart->read_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->read_buf = msg;
    uart->read_length = length;
    uart->read_pos = 0;

    etimer_set(&uart->read_timer, timeout);

    // Await completion or timeout.
    PT_WAIT_UNTIL(pt, ({
        // On every re-entry to the async read, drain the ring buffer
        // into the current read buffer. This ensures that we use
        // all available data if there have been multiple polls since our last
        // re-entry. If there is already enough data in the buffer, this
        // protothread completes right away without yielding once first.
        while (uart->read_pos < uart->read_length) {
            int c = ringbuf_get(&uart->rx_buf);
            if (c == -1) {
                break;
            }
            uart->read_buf[uart->read_pos++] = c;
        }
        uart->read_pos == uart->read_length || etimer_expired(&uart->read_timer);
    }));

    // Set exit status based on completion condition.
    if (etimer_expired(&uart->read_timer)) {
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->read_timer);
        *err = PBIO_SUCCESS;
    }
    uart->read_buf = NULL;

    PT_END(pt);
}

PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    PT_BEGIN(pt);

    if (uart->write_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    etimer_set(&uart->write_timer, timeout);

    LL_USART_EnableIT_TXE(uart->pdata->uart);

    // Await completion or timeout.
    PT_WAIT_UNTIL(pt, uart->write_pos == uart->write_length || etimer_expired(&uart->write_timer));

    // Set exit status based on completion condition.
    if (etimer_expired(&uart->write_timer)) {
        LL_USART_DisableIT_TXE(uart->pdata->uart);
        LL_USART_DisableIT_TC(uart->pdata->uart);
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->write_timer);
        *err = PBIO_SUCCESS;
    }

    uart->write_buf = NULL;

    PT_END(pt);
}

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
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
}

void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    // If a process was exited while an operation was in progress this is
    // normally an error, and the process may call flush when it is restarted
    // to clear the state.
    uart->write_buf = NULL;
    uart->write_length = 0;
    uart->write_pos = 0;
    uart->read_buf = NULL;
    uart->read_length = 0;
    uart->read_pos = 0;
    // Discard all received bytes.
    while (ringbuf_get(&uart->rx_buf) != -1) {
        ;
    }
}

void pbdrv_uart_stm32f4_ll_irq_handle_irq(uint8_t id) {
    pbdrv_uart_t *uart = &pbdrv_uart[id];
    USART_TypeDef *USARTx = uart->pdata->uart;
    uint32_t sr = USARTx->SR;

    if (sr & USART_SR_RXNE) {
        ringbuf_put(&uart->rx_buf, LL_USART_ReceiveData8(USARTx));
        // Poll parent process for each received byte, since the IRQ handler
        // has no awareness of the expected length of the read operation.
        if (uart->poll_callback) {
            uart->poll_callback(&uart->uart_dev);
        }
    }

    if (sr & USART_SR_ORE) {
        // clears interrupt
        LL_USART_ReceiveData8(USARTx);
    }

    if (USARTx->CR1 & USART_CR1_TXEIE && sr & USART_SR_TXE && uart->write_buf) {
        LL_USART_TransmitData8(USARTx, uart->write_buf[uart->write_pos++]);
        // When all bytes have been written, wait for the Tx complete interrupt.
        if (uart->write_pos == uart->write_length) {
            LL_USART_DisableIT_TXE(USARTx);
            LL_USART_EnableIT_TC(USARTx);
        }
        // No need to poll parent process here: the interrupts will keep it
        // going until the whole buffer has been written out. The completion
        // event below will trigger the poll parent process.
    }

    if (USARTx->CR1 & USART_CR1_TCIE && sr & USART_SR_TC) {
        LL_USART_DisableIT_TC(USARTx);
        // Poll parent process to indicate the write operation is complete.
        if (uart->poll_callback) {
            uart->poll_callback(&uart->uart_dev);
        }
    }
}

// Currently not used
void handle_exit(void) {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F4_LL_IRQ_NUM_UART; i++) {
        const pbdrv_uart_stm32f4_ll_irq_platform_data_t *pdata = &pbdrv_uart_stm32f4_ll_irq_platform_data[i];
        LL_USART_Disable(pdata->uart);
        NVIC_DisableIRQ(pdata->irq);
    }
}

void pbdrv_uart_init(void) {

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
}

#endif // PBDRV_CONFIG_UART_STM32F4_LL_IRQ
