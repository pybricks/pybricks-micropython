// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_UART_STM32_HAL

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <contiki.h>

#include <pbdrv/uart.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "../../src/processes.h"

#include STM32_HAL_H
#include "uart_stm32_hal.h"

typedef struct {
    pbdrv_uart_dev_t uart_dev;
    UART_HandleTypeDef huart;
    struct etimer rx_timer;
    struct etimer tx_timer;
    volatile pbio_error_t rx_result;
    volatile pbio_error_t tx_result;
    uint8_t irq;
} pbdrv_uart_t;

static pbdrv_uart_t pbdrv_uart[PBDRV_CONFIG_UART_STM32_HAL_NUM_UART];

PROCESS(pbdrv_uart_process, "UART");

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32_HAL_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (HAL_UART_GetState(&pbdrv_uart[id].huart) == HAL_UART_STATE_RESET) {
        return PBIO_ERROR_AGAIN;
    }

    *uart_dev = &pbdrv_uart[id].uart_dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    HAL_StatusTypeDef ret;

    if (uart->huart.RxState != HAL_UART_STATE_READY) {
        return PBIO_ERROR_AGAIN;
    }

    // can't receive if overrun flag is set
    __HAL_UART_CLEAR_OREFLAG(&uart->huart);

    ret = HAL_UART_Receive_IT(&uart->huart, msg, length);
    if (ret == HAL_ERROR) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (ret == HAL_BUSY) {
        return PBIO_ERROR_AGAIN;
    }

    etimer_set(&uart->rx_timer, clock_from_msec(timeout));
    uart->rx_result = PBIO_ERROR_AGAIN;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    pbio_error_t err = uart->rx_result; // read once since interrupt can modify it

    if (err != PBIO_ERROR_AGAIN) {
        etimer_stop(&uart->rx_timer);
    } else if (etimer_expired(&uart->rx_timer)) {
        // NB: This function can be blocking when using DMA - if we switch to
        // DMA, the timout will need to be reworked.
        HAL_UART_AbortReceive(&uart->huart);
        err = PBIO_ERROR_TIMEDOUT;
    }

    return err;
}

void pbdrv_uart_read_cancel(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    HAL_UART_AbortReceive_IT(&uart->huart);
}

pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    HAL_StatusTypeDef ret;

    ret = HAL_UART_Transmit_IT(&uart->huart, msg, length);
    if (ret == HAL_ERROR) {
        return PBIO_ERROR_INVALID_ARG;
    }
    if (ret == HAL_BUSY) {
        return PBIO_ERROR_AGAIN;
    }

    etimer_set(&uart->tx_timer, clock_from_msec(timeout));
    uart->tx_result = PBIO_ERROR_AGAIN;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);
    pbio_error_t err = uart->tx_result; // read once since interrupt can modify it

    if (err != PBIO_ERROR_AGAIN) {
        etimer_stop(&uart->tx_timer);
    } else if (etimer_expired(&uart->tx_timer)) {
        // NB: This function can be blocking when using DMA - if we switch to
        // DMA, the timout will need to be reworked.
        HAL_UART_AbortTransmit(&uart->huart);
        err = PBIO_ERROR_TIMEDOUT;
    }

    return err;
}

void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart_dev) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    HAL_UART_AbortTransmit_IT(&uart->huart);
}

pbio_error_t pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(uart_dev, pbdrv_uart_t, uart_dev);

    if (HAL_UART_GetState(&uart->huart) != HAL_UART_STATE_READY) {
        return PBIO_ERROR_AGAIN;
    }

    uart->huart.Init.BaudRate = baud;
    // REVISIT: This is a potentially blocking function
    HAL_UART_Init(&uart->huart);

    return PBIO_SUCCESS;
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, huart);

    if (uart->huart.ErrorCode & HAL_UART_ERROR_ORE) {
        uart->rx_result = PBIO_ERROR_IO;
    }
    process_poll(&pbdrv_uart_process);
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, huart);

    uart->rx_result = PBIO_SUCCESS;
    process_poll(&pbdrv_uart_process);
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_AbortReceiveCpltCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, huart);

    uart->rx_result = PBIO_ERROR_CANCELED;
    process_poll(&pbdrv_uart_process);
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, huart);

    uart->tx_result = PBIO_SUCCESS;
    process_poll(&pbdrv_uart_process);
}

// overrides weak function in stm32f4xx_hal_uart.c
void HAL_UART_AbortTransmitCpltCallback(UART_HandleTypeDef *huart) {
    pbdrv_uart_t *uart = PBIO_CONTAINER_OF(huart, pbdrv_uart_t, huart);

    uart->tx_result = PBIO_ERROR_CANCELED;
    process_poll(&pbdrv_uart_process);
}

void pbdrv_uart_stm32_hal_handle_irq(uint8_t id) {
    HAL_UART_IRQHandler(&pbdrv_uart[id].huart);
}

static void handle_poll() {
    process_post(PROCESS_BROADCAST, PROCESS_EVENT_COM, NULL);
}

static void handle_exit() {
    for (int i = 0; i < PBDRV_CONFIG_UART_STM32_HAL_NUM_UART; i++) {
        pbdrv_uart_t *uart = &pbdrv_uart[i];
        HAL_NVIC_DisableIRQ(uart->irq);
        HAL_UART_DeInit(&uart->huart);
    }
}

PROCESS_THREAD(pbdrv_uart_process, ev, data) {
    PROCESS_POLLHANDLER(handle_poll());
    PROCESS_EXITHANDLER(handle_exit());

    PROCESS_BEGIN();

    for (int i = 0; i < PBDRV_CONFIG_UART_STM32_HAL_NUM_UART; i++) {
        const pbdrv_uart_stm32_hal_platform_data_t *pdata = &pbdrv_uart_stm32_hal_platform_data[i];
        pbdrv_uart_t *uart = &pbdrv_uart[i];

        // NB: have to init with 115200 on STM32L4 LPUART1, otherwise
        // HAL_UART_Init() will fail because it has limited baud rate
        // (minimum speed is 19200 with 80MHz clock).
        uart->huart.Instance = pdata->uart,
        uart->huart.Init.BaudRate = 115200,
        uart->huart.Init.WordLength = UART_WORDLENGTH_8B,
        uart->huart.Init.StopBits = UART_STOPBITS_1,
        uart->huart.Init.Parity = UART_PARITY_NONE,
        uart->huart.Init.Mode = UART_MODE_TX_RX,
        uart->huart.Init.HwFlowCtl = UART_HWCONTROL_NONE,
        uart->huart.Init.OverSampling = UART_OVERSAMPLING_16,
        uart->irq = pdata->irq,
        HAL_UART_Init(&pbdrv_uart[i].huart);
        HAL_NVIC_SetPriority(uart->irq, 0, 0);
        HAL_NVIC_EnableIRQ(uart->irq);
    }

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_UART_STM32_HAL
