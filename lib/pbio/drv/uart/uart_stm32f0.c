// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// This driver is for UARTs on STM32F0 MCUs. It provides async read and write
// functions for sending and receive data and allows changing the baud rate.
// There are no hardware buffers on the UARTs, so we implement a ring buffer
// to queue received data until it is read. No extra buffering is needed for
// transmitting.

#include "pbdrv/config.h"

#if PBDRV_CONFIG_UART_STM32F0

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <contiki.h>

#include <pbdrv/uart.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "../core.h"

#include "stm32f0xx.h"
#include "uart_stm32f0.h"

#define UART_RING_BUF_SIZE 32   // must be a power of 2!

struct _pbdrv_uart_dev_t {
    USART_TypeDef *USART;
    uint8_t rx_ring_buf[UART_RING_BUF_SIZE];
    volatile uint8_t rx_ring_buf_head;
    uint8_t rx_ring_buf_tail;
    uint8_t *rx_buf;
    uint8_t rx_buf_size;
    uint8_t rx_buf_index;
    uint8_t *tx_buf;
    uint8_t tx_buf_size;
    uint8_t tx_buf_index;
    struct etimer rx_timer;
    struct etimer tx_timer;
    uint8_t irq;
    bool initialized;
    /**
     * Parent process that handles incoming data.
     *
     * All protothreads in this module run within that process.
     */
    struct process *parent_process;
};

static pbdrv_uart_dev_t uart_devs[PBDRV_CONFIG_UART_STM32F0_NUM_UART];

pbio_error_t pbdrv_uart_get_instance(uint8_t id, struct process *parent_process, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32F0_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    pbdrv_uart_dev_t *dev = &uart_devs[id];
    if (!dev->initialized) {
        return PBIO_ERROR_AGAIN;
    }
    dev->parent_process = parent_process;
    *uart_dev = dev;
    return PBIO_SUCCESS;
}

PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    PT_BEGIN(pt);
    if (!msg || !length) {
        *err = PBIO_ERROR_INVALID_ARG;
        PT_EXIT(pt);
    }

    if (uart->rx_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->rx_buf = msg;
    uart->rx_buf_size = length;
    uart->rx_buf_index = 0;

    if (timeout) {
        etimer_set(&uart->rx_timer, timeout);
    }

    // Await completion or timeout.
    PT_WAIT_UNTIL(pt, ({
        // On every re-entry to the async read, drain the ring buffer
        // into the current read buffer. This ensures that we use
        // all available data if there have been multiple polls since our last
        // re-entry. If there is already enough data in the buffer, this
        // protothread completes right away without yielding once first.
        while (uart->rx_ring_buf_head != uart->rx_ring_buf_tail && uart->rx_buf_index < uart->rx_buf_size) {
            uart->rx_buf[uart->rx_buf_index++] = uart->rx_ring_buf[uart->rx_ring_buf_tail];
            uart->rx_ring_buf_tail = (uart->rx_ring_buf_tail + 1) & (UART_RING_BUF_SIZE - 1);
        }
        uart->rx_buf_index == uart->rx_buf_size || (timeout && etimer_expired(&uart->rx_timer));
    }));

    if (timeout && etimer_expired(&uart->rx_timer)) {
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->rx_timer);
        *err = PBIO_SUCCESS;
    }
    uart->rx_buf = NULL;

    PT_END(pt);
}

PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {

    PT_BEGIN(pt);

    if (!msg || !length) {
        *err = PBIO_ERROR_INVALID_ARG;
        PT_EXIT(pt);
    }

    if (uart->tx_buf) {
        *err = PBIO_ERROR_BUSY;
        PT_EXIT(pt);
    }

    uart->tx_buf = msg;
    uart->tx_buf_size = length;
    uart->tx_buf_index = 0;

    if (timeout) {
        etimer_set(&uart->tx_timer, timeout);
    }

    uart->USART->CR1 |= USART_CR1_TXEIE;

    // Await completion or timeout.
    PT_WAIT_UNTIL(pt, uart->tx_buf_index == uart->tx_buf_size || (timeout && etimer_expired(&uart->tx_timer)));

    if (timeout && etimer_expired(&uart->tx_timer)) {
        uart->USART->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
        *err = PBIO_ERROR_TIMEDOUT;
    } else {
        etimer_stop(&uart->tx_timer);
        *err = PBIO_SUCCESS;
    }
    uart->tx_buf = NULL;

    PT_END(pt);
}

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart, uint32_t baud) {
    uart->USART->BRR = PBDRV_CONFIG_SYS_CLOCK_RATE / baud;
}

void pbdrv_uart_flush(pbdrv_uart_dev_t *uart) {
    uart->rx_buf = NULL;
    uart->tx_buf = NULL;
    uart->rx_ring_buf_head = 0;
    uart->rx_ring_buf_tail = 0;
    uart->rx_buf_size = 0;
    uart->rx_buf_index = 0;
}

void pbdrv_uart_stm32f0_handle_irq(uint8_t id) {
    pbdrv_uart_dev_t *uart = &uart_devs[id];
    uint32_t isr = uart->USART->ISR;

    // receive next byte
    if (isr & USART_ISR_RXNE) {
        // REVISIT: Do we need to have an overrun error when the ring buffer gets full?
        uart->rx_ring_buf[uart->rx_ring_buf_head] = uart->USART->RDR;
        uart->rx_ring_buf_head = (uart->rx_ring_buf_head + 1) & (UART_RING_BUF_SIZE - 1);
        process_poll(uart->parent_process);
    }

    // transmit next byte
    if (uart->USART->CR1 & USART_CR1_TXEIE && isr & USART_ISR_TXE) {
        uart->USART->TDR = uart->tx_buf[uart->tx_buf_index++];
        if (uart->tx_buf_index == uart->tx_buf_size) {
            // all bytes have been sent, so enable transmit complete interrupt
            uart->USART->CR1 &= ~USART_CR1_TXEIE;
            uart->USART->CR1 |= USART_CR1_TCIE;
        }
        // No poll needed here. The interrupt will keep going until
        // the whole buffer has been written out. The completion event below
        // will poll the parent process.
    }

    // transmission complete
    if (uart->USART->CR1 & USART_CR1_TCIE && isr & USART_ISR_TC) {
        uart->USART->CR1 &= ~USART_CR1_TCIE;
        process_poll(uart->parent_process);
    }
}

void pbdrv_uart_stop(pbdrv_uart_dev_t *uart) {
    NVIC_DisableIRQ(uart->irq);
}

void pbdrv_uart_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F0_NUM_UART; i++) {
        const pbdrv_uart_stm32f0_platform_data_t *pdata = &pbdrv_uart_stm32f0_platform_data[i];
        pbdrv_uart_dev_t *uart = &uart_devs[i];

        uart->USART = pdata->uart,
        uart->irq = pdata->irq,

        uart->USART->CR3 |= USART_CR3_OVRDIS;
        uart->USART->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
        NVIC_SetPriority(uart->irq, 0);
        NVIC_EnableIRQ(uart->irq);

        uart->initialized = true;
    }
}

#endif // PBDRV_CONFIG_UART_STM32F0
