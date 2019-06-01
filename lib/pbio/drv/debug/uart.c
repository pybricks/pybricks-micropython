// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pbdrv/config.h"
#include "pbio/error.h"
#include "pbio/event.h"
#include "pbio/port.h"
#include "pbio/uartdev.h"
#include "pbio/util.h"
#include "pbsys/sys.h"
#include "sys/process.h"
#include "../../src/processes.h"

#define USE_HAL_DRIVER
#include "stm32f4xx.h"

#define TX_BUF_SIZE 32  // must be power of 2!
#define RX_BUF_SIZE 64  // must be power of 2!

typedef struct {
    UART_HandleTypeDef handle;
    const uint8_t irq;
    uint8_t tx_buf[TX_BUF_SIZE];
    uint8_t rx_buf[RX_BUF_SIZE];
    uint8_t tx_buf_head;
    volatile uint8_t tx_buf_tail;
    uint8_t tx_byte;
    volatile uint8_t rx_buf_head;
    uint8_t rx_buf_tail;
    uint8_t rx_byte;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_NUM_IO_PORT] = {
    [0] = {
        .handle.Instance = USART2,
        .handle.Init.BaudRate = 2400,
        .handle.Init.WordLength = UART_WORDLENGTH_8B,
        .handle.Init.StopBits = UART_STOPBITS_1,
        .handle.Init.Parity = UART_PARITY_NONE,
        .handle.Init.Mode = UART_MODE_TX_RX,
        .handle.Init.HwFlowCtl = UART_HWCONTROL_NONE,
        .handle.Init.OverSampling = UART_OVERSAMPLING_16,
        .irq = USART2_IRQn,
    },
};

PROCESS(pbdrv_uart_process, "UART");

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, handle);
    uint8_t new_head;

    new_head = (uart->rx_buf_head + 1) & (RX_BUF_SIZE - 1);
    if (new_head == uart->rx_buf_tail) {
        // buffer overrun
        // REVISIT: ignoring for now - will lose characters
        HAL_UART_Receive_IT(&uart->handle, &uart->rx_byte, 1);
        return;
    }
    uart->rx_buf[uart->rx_buf_head] = uart->rx_byte;
    uart->rx_buf_head = new_head;
    HAL_UART_Receive_IT(&uart->handle, &uart->rx_byte, 1);

    process_poll(&pbdrv_uart_process);
}

static pbio_error_t _pbdrv_uart_get_char(pbio_port_t port, uint8_t *c, bool peek) {
    pbdrv_uart_t *uart;

    switch (port) {
    case PBIO_PORT_1:
        uart = &pbdrv_uart[port - PBDRV_CONFIG_FIRST_IO_PORT];
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    if (uart->rx_buf_head == uart->rx_buf_tail) {
        return PBIO_ERROR_AGAIN;
    }

    *c = uart->rx_buf[uart->rx_buf_tail];
    if (!peek) {
        uart->rx_buf_tail = (uart->rx_buf_tail + 1) & (RX_BUF_SIZE - 1);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_peek_char(pbio_port_t port, uint8_t *c) {
    return _pbdrv_uart_get_char(port, c, true);
}

pbio_error_t pbdrv_uart_get_char(pbio_port_t port, uint8_t *c) {
    return _pbdrv_uart_get_char(port, c, false);
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, handle);

    if (uart->tx_buf_tail != uart->tx_buf_head) {
        // there is still more data to send
        uart->tx_byte = uart->tx_buf[uart->tx_buf_tail];
        uart->tx_buf_tail = (uart->tx_buf_tail + 1) & (TX_BUF_SIZE - 1);
        HAL_UART_Transmit_IT(&uart->handle, &uart->tx_byte, 1);
    }
}

pbio_error_t pbdrv_uart_put_char(pbio_port_t port, uint8_t c) {
    pbdrv_uart_t *uart;
    uint8_t new_head;

    switch (port) {
    case PBIO_PORT_1:
        uart = &pbdrv_uart[port - PBDRV_CONFIG_FIRST_IO_PORT];
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    if (uart->tx_buf_head == uart->tx_buf_tail) {
        uart->tx_byte = c;
        HAL_UART_Transmit_IT(&uart->handle, &uart->tx_byte, 1);
    }
    else {
        // otherwise queue it in the ring buffer
        new_head = (uart->tx_buf_head + 1) & (TX_BUF_SIZE - 1);
        if (new_head == uart->tx_buf_tail) {
            // buffer is full
            return PBIO_ERROR_AGAIN;
        }
        uart->tx_buf[uart->tx_buf_head] = c;
        uart->tx_buf_head = new_head;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_set_baud_rate(pbio_port_t port, uint32_t baud) {
    pbdrv_uart_t *uart;

    switch (port) {
    case PBIO_PORT_1:
        uart = &pbdrv_uart[port - PBDRV_CONFIG_FIRST_IO_PORT];
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    uart->handle.Init.BaudRate = baud;
    HAL_UART_Init(&uart->handle);
    HAL_UART_Receive_IT(&uart->handle, &uart->rx_byte, 1);

    return PBIO_SUCCESS;
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, handle);

    // clocks are enabled in sys.c
    // pin mux is handled in ioport.c

    HAL_NVIC_SetPriority(uart->irq, 1, 0);
    HAL_NVIC_EnableIRQ(uart->irq);
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, handle);

    HAL_NVIC_DisableIRQ(uart->irq);
}

// overrides weak function in setup.m
void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&pbdrv_uart[0].handle);
}

static void handle_poll() {
    uint8_t i, c;

    for (i = PBDRV_CONFIG_FIRST_IO_PORT; i <= PBDRV_CONFIG_LAST_IO_PORT; i++) {
        while (pbdrv_uart_get_char(i, &c) == PBIO_SUCCESS) {
            pbio_event_uart_rx_data_t rx = { .port = i, .byte = c };
            process_post_synch(&pbio_uartdev_process, PBIO_EVENT_UART_RX, &rx);
        }
    }
}

static void handle_exit() {
    for (int i = 0; i < PBDRV_CONFIG_NUM_IO_PORT; i++) {
        HAL_UART_DeInit(&pbdrv_uart[i].handle);
    }

}

PROCESS_THREAD(pbdrv_uart_process, ev, data) {
    PROCESS_POLLHANDLER(handle_poll());
    PROCESS_EXITHANDLER(handle_exit());

    PROCESS_BEGIN();

    for (int i = 0; i < PBDRV_CONFIG_NUM_IO_PORT; i++) {
        HAL_UART_Init(&pbdrv_uart[i].handle);
        HAL_UART_Receive_IT(&pbdrv_uart[i].handle, &pbdrv_uart[i].rx_byte, 1);
    }

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}
