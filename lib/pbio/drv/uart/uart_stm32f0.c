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

#include "../../src/processes.h"

#include "stm32f0xx.h"
#include "uart_stm32f0.h"

#define UART_RING_BUF_SIZE 32   // must be a power of 2!

typedef struct {
    pbdrv_uart_dev_t uart_dev;
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
    volatile pbio_error_t rx_result;
    volatile pbio_error_t tx_result;
    uint8_t irq;
    bool initalized;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_UART_STM32F0_NUM_UART];

PROCESS(pbdrv_uart_process, "UART");

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32F0_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (!pbdrv_uart[id].initalized) {
        return PBIO_ERROR_AGAIN;
    }

    *uart_dev = &pbdrv_uart[id].uart_dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    if (!msg || !length) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (uart->rx_buf) {
        return PBIO_ERROR_AGAIN;
    }

    uart->rx_buf = msg;
    uart->rx_buf_size = length;
    uart->rx_buf_index = 0;
    uart->rx_result = PBIO_ERROR_AGAIN;

    etimer_set(&uart->rx_timer, clock_from_msec(timeout));

    process_poll(&pbdrv_uart_process);

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    pbio_error_t err = uart->rx_result; // read once since interrupt can modify it

    if (uart->rx_buf == NULL) {
        // begin was not called first
        return PBIO_ERROR_INVALID_OP;
    }

    if (err != PBIO_ERROR_AGAIN) {
        etimer_stop(&uart->rx_timer);
        uart->rx_buf = NULL;
    } else if (etimer_expired(&uart->rx_timer)) {
        err = PBIO_ERROR_TIMEDOUT;
        uart->rx_buf = NULL;
    }

    return err;
}

void pbdrv_uart_read_cancel(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    uart->rx_result = PBIO_ERROR_CANCELED;
}

pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    if (!msg || !length) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (uart->tx_buf) {
        return PBIO_ERROR_AGAIN;
    }

    uart->tx_buf = msg;
    uart->tx_buf_size = length;
    uart->tx_buf_index = 0;
    uart->tx_result = PBIO_ERROR_AGAIN;

    etimer_set(&uart->tx_timer, clock_from_msec(timeout));

    uart->USART->CR1 |= USART_CR1_TXEIE;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    pbio_error_t err = uart->tx_result; // read once since interrupt can modify it

    if (uart->tx_buf == NULL) {
        // begin was not called first
        return PBIO_ERROR_INVALID_OP;
    }

    if (err != PBIO_ERROR_AGAIN) {
        etimer_stop(&uart->tx_timer);
        uart->tx_buf = NULL;
    } else if (etimer_expired(&uart->tx_timer)) {
        uart->USART->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
        err = PBIO_ERROR_TIMEDOUT;
        uart->tx_buf = NULL;
    }

    return err;
}

void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    uart->USART->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
    uart->tx_result = PBIO_ERROR_CANCELED;
}

pbio_error_t pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    if (uart->tx_buf || uart->rx_buf) {
        return PBIO_ERROR_AGAIN;
    }

    uart->USART->BRR = PBDRV_CONFIG_SYS_CLOCK_RATE / baud;

    return PBIO_SUCCESS;
}

void pbdrv_uart_stm32f0_handle_irq(uint8_t id) {
    pbdrv_uart_t *uart = &pbdrv_uart[id];

    // receive next byte
    if (uart->USART->ISR & USART_ISR_RXNE) {
        // REVISIT: Do we need to have an overrun error when the ring buffer gets full?
        uart->rx_ring_buf[uart->rx_ring_buf_head] = uart->USART->RDR;
        uart->rx_ring_buf_head = (uart->rx_ring_buf_head + 1) & (UART_RING_BUF_SIZE - 1);
        process_poll(&pbdrv_uart_process);
    }

    // transmit next byte
    if (uart->USART->CR1 & USART_CR1_TXEIE && uart->USART->ISR & USART_ISR_TXE) {
        uart->USART->TDR = uart->tx_buf[uart->tx_buf_index++];
        if (uart->tx_buf_index == uart->tx_buf_size) {
            // all bytes have been sent, so enable transmit complete interrupt
            uart->USART->CR1 &= ~USART_CR1_TXEIE;
            uart->USART->CR1 |= USART_CR1_TCIE;
        }
    }

    // transmission complete
    if (uart->USART->CR1 & USART_CR1_TCIE && uart->USART->ISR & USART_ISR_TC) {
        uart->USART->CR1 &= ~USART_CR1_TCIE;
        uart->tx_result = PBIO_SUCCESS;
        process_poll(&pbdrv_uart_process);
    }
}

static void handle_poll() {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F0_NUM_UART; i++) {
        pbdrv_uart_t *uart = &pbdrv_uart[i];

        // if receive is pending and we have not received all bytes yet...
        if (uart->rx_buf && uart->rx_result == PBIO_ERROR_AGAIN && uart->rx_buf_index < uart->rx_buf_size) {
            // copy all available bytes to rx_buf
            while (uart->rx_ring_buf_head != uart->rx_ring_buf_tail) {
                uart->rx_buf[uart->rx_buf_index++] = uart->rx_ring_buf[uart->rx_ring_buf_tail];
                uart->rx_ring_buf_tail = (uart->rx_ring_buf_tail + 1) & (UART_RING_BUF_SIZE - 1);
                // when rx_buf is full, send out a notification
                if (uart->rx_buf_index == uart->rx_buf_size) {
                    uart->rx_result = PBIO_SUCCESS;
                    process_post(PROCESS_BROADCAST, PROCESS_EVENT_COM, NULL);
                    break;
                }
            }
        }

        if (uart->tx_buf && uart->tx_buf_index == uart->tx_buf_size) {
            // TODO: this should only be sent once per write_begin
            process_post(PROCESS_BROADCAST, PROCESS_EVENT_COM, NULL);
        }
    }
}

static void handle_exit() {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F0_NUM_UART; i++) {
        pbdrv_uart_t *uart = &pbdrv_uart[i];
        NVIC_DisableIRQ(uart->irq);
    }
}

PROCESS_THREAD(pbdrv_uart_process, ev, data) {
    PROCESS_POLLHANDLER(handle_poll());
    PROCESS_EXITHANDLER(handle_exit());

    PROCESS_BEGIN();

    for (int i = 0; i < PBDRV_CONFIG_UART_STM32F0_NUM_UART; i++) {
        const pbdrv_uart_stm32f0_platform_data_t *pdata = &pbdrv_uart_stm32f0_platform_data[i];
        pbdrv_uart_t *uart = &pbdrv_uart[i];

        uart->USART = pdata->uart,
        uart->irq = pdata->irq,

        uart->USART->CR3 |= USART_CR3_OVRDIS;
        uart->USART->CR1 |= USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
        NVIC_SetPriority(uart->irq, 0);
        NVIC_EnableIRQ(uart->irq);

        uart->initalized = true;
    }

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_UART_STM32F0
