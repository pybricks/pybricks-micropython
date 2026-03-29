// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

// UART driver for STM32F4x using IRQ.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_STM32_LL_IRQ

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include STM32_LL_RCC_H
#include STM32_LL_USART_H

#include <pbdrv/uart.h>

#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include <lwrb/lwrb.h>

#include "./uart_stm32_ll_irq.h"

#define RX_DATA_SIZE 64 // must be power of 2 for ring buffer!

struct _pbdrv_uart_dev_t {
    /** Platform-specific data */
    const pbdrv_uart_stm32_ll_irq_platform_data_t *pdata;
    /** Circular buffer for caching received bytes. */
    lwrb_t rx_buf;
    /** Timer for read timeout. */
    pbio_os_timer_t read_timer;
    /** Timer for write timeout. */
    pbio_os_timer_t write_timer;
    /** The buffer of the ongoing async read function. */
    uint8_t *read_buf;
    /** The length of read_buf in bytes. */
    uint32_t read_length;
    /** The current position in read_buf. */
    uint32_t read_pos;
    /** The buffer of the ongoing write function. */
    const uint8_t *write_buf;
    /** The length of write_buf in bytes. */
    uint32_t write_length;
    /** The current position in write_buf. */
    volatile uint32_t write_pos;
};

static pbdrv_uart_dev_t uart_devs[PBDRV_CONFIG_UART_STM32_LL_IRQ_NUM_UART];
static uint8_t pbdrv_uart_rx_data[PBDRV_CONFIG_UART_STM32_LL_IRQ_NUM_UART][RX_DATA_SIZE];

pbio_error_t pbdrv_uart_get_instance(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    if (id >= PBDRV_CONFIG_UART_STM32_LL_IRQ_NUM_UART) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbdrv_uart_dev_t *dev = &uart_devs[id];
    if (!dev->pdata) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }
    *uart_dev = dev;
    return PBIO_SUCCESS;
}

uint32_t pbdrv_uart_in_waiting(pbdrv_uart_dev_t *uart_dev) {
    return lwrb_get_full(&uart_dev->rx_buf);
}

pbio_error_t pbdrv_uart_read(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, uint8_t *msg, uint32_t length, uint32_t timeout) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (uart->read_buf) {
        return PBIO_ERROR_BUSY;
    }

    uart->read_buf = msg;
    uart->read_length = length;
    uart->read_pos = 0;

    if (timeout) {
        pbio_os_timer_set(&uart->read_timer, timeout);
    }

    // Await completion or timeout.
    PBIO_OS_AWAIT_UNTIL(state, ({
        // On every re-entry to the async read, drain the ring buffer
        // into the current read buffer. This ensures that we use
        // all available data if there have been multiple polls since our last
        // re-entry. If there is already enough data in the buffer, this
        // protothread completes right away without yielding once first.
        uart->read_pos += lwrb_read(&uart->rx_buf, &uart->read_buf[uart->read_pos], uart->read_length - uart->read_pos);
        uart->read_pos == uart->read_length || (timeout && pbio_os_timer_is_expired(&uart->read_timer));
    }));

    uart->read_buf = NULL;

    if (timeout && pbio_os_timer_is_expired(&uart->read_timer)) {
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_uart_write(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, const uint8_t *msg, uint32_t length, uint32_t timeout) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (uart->write_buf) {
        return PBIO_ERROR_BUSY;
    }

    uart->write_buf = msg;
    uart->write_length = length;
    uart->write_pos = 0;

    if (timeout) {
        pbio_os_timer_set(&uart->write_timer, timeout);
    }

    LL_USART_EnableIT_TXE(uart->pdata->uart);

    // Await completion or timeout.
    PBIO_OS_AWAIT_UNTIL(state, uart->write_pos == uart->write_length || (timeout && pbio_os_timer_is_expired(&uart->write_timer)));

    uart->write_buf = NULL;

    // Set exit status based on completion condition.
    if ((timeout && pbio_os_timer_is_expired(&uart->write_timer))) {
        LL_USART_DisableIT_TXE(uart->pdata->uart);
        LL_USART_DisableIT_TC(uart->pdata->uart);
        return PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#if defined(STM32F4)
void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart, uint32_t baud) {

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
#elif defined(STM32H5)
void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart, uint32_t baud) {
    USART_TypeDef *USARTx = uart->pdata->uart;
    uint32_t periphclk = LL_RCC_PERIPH_FREQUENCY_NO;

    if (USARTx == USART1) {
        periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART1_CLKSOURCE);
    } else if (USARTx == USART2) {
        periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART2_CLKSOURCE);
    } else if (USARTx == USART3) {
        periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART3_CLKSOURCE);
    }
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
    #if defined(USART6)
    else if (USARTx == USART6) {
        periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART6_CLKSOURCE);
    }
    #endif /* USART6 */
    #if defined(UART7)
    else if (USARTx == UART7) {
        periphclk = LL_RCC_GetUARTClockFreq(LL_RCC_UART7_CLKSOURCE);
    }
    #endif /* UART7 */
    #if defined(UART8)
    else if (USARTx == UART8) {
        periphclk = LL_RCC_GetUARTClockFreq(LL_RCC_UART8_CLKSOURCE);
    }
    #endif /* UART8 */
    #if defined(UART9)
    else if (USARTx == UART9) {
        periphclk = LL_RCC_GetUARTClockFreq(LL_RCC_UART9_CLKSOURCE);
    }
    #endif /* UART9 */
    #if defined(USART10)
    else if (USARTx == USART10) {
        periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART10_CLKSOURCE);
    }
    #endif /* USART10 */
    #if defined(USART11)
    else if (USARTx == USART11) {
        periphclk = LL_RCC_GetUSARTClockFreq(LL_RCC_USART11_CLKSOURCE);
    }
    #endif /* USART11 */
    #if defined(UART12)
    else if (USARTx == UART12) {
        periphclk = LL_RCC_GetUARTClockFreq(LL_RCC_UART12_CLKSOURCE);
    }
    #endif /* UART12 */

    // TODO: confirm that we don't need different prescalar.
    // i.e. assert_param(IS_LL_USART_BRR_MIN(USARTx->BRR))
    LL_USART_SetBaudRate(USARTx, periphclk, LL_USART_PRESCALER_DIV1, LL_USART_OVERSAMPLING_16, baud);
    LL_USART_SetPrescaler(USARTx, LL_USART_PRESCALER_DIV1);
}
#else
#error "unsupported MCU for btstack_stm32_hal_set_baudrate()"
#endif

void pbdrv_uart_flush(pbdrv_uart_dev_t *uart) {
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
    lwrb_reset(&uart->rx_buf);
}

void pbdrv_uart_stm32_ll_irq_handle_irq(uint8_t id) {
    pbdrv_uart_dev_t *uart = &uart_devs[id];
    USART_TypeDef *USARTx = uart->pdata->uart;
    #if defined(STM32H5)
    uint32_t sr = USARTx->ISR;
    #else
    uint32_t sr = USARTx->SR;
    #endif

    #if defined(STM32H5)
    if (sr & USART_ISR_RXNE) {
    #else
    if (sr & USART_SR_RXNE) {
        #endif
        uint8_t c = LL_USART_ReceiveData8(USARTx);
        lwrb_write(&uart->rx_buf, &c, 1);
        // Poll parent process for each received byte, since the IRQ handler
        // has no awareness of the expected length of the read operation.
        pbio_os_request_poll();

    }

    #if defined(STM32H5)
    if (sr & USART_ISR_ORE) {
    #else
    if (sr & USART_SR_ORE) {
        #endif
        // clears interrupt
        LL_USART_ReceiveData8(USARTx);
    }

    #if defined(STM32H5)
    if (USARTx->CR1 & USART_CR1_TXEIE && sr & USART_ISR_TXE && uart->write_buf) {
    #else
    if (USARTx->CR1 & USART_CR1_TXEIE && sr & USART_SR_TXE && uart->write_buf) {
        #endif
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

    #if defined(STM32H5)
    if (USARTx->CR1 & USART_CR1_TCIE && sr & USART_ISR_TC) {
    #else
    if (USARTx->CR1 & USART_CR1_TCIE && sr & USART_SR_TC) {
        #endif
        LL_USART_DisableIT_TC(USARTx);
        // Poll parent process to indicate the write operation is complete.
        pbio_os_request_poll();
    }
}

void pbdrv_uart_stop(pbdrv_uart_dev_t *uart) {
    LL_USART_Disable(uart->pdata->uart);
    NVIC_DisableIRQ(uart->pdata->irq);
}

void pbdrv_uart_init(void) {

    for (int i = 0; i < PBDRV_CONFIG_UART_STM32_LL_IRQ_NUM_UART; i++) {
        const pbdrv_uart_stm32_ll_irq_platform_data_t *pdata = &pbdrv_uart_stm32_ll_irq_platform_data[i];
        uint8_t *rx_data = pbdrv_uart_rx_data[i];
        pbdrv_uart_dev_t *uart = &uart_devs[i];
        uart->pdata = pdata;
        lwrb_init(&uart->rx_buf, rx_data, RX_DATA_SIZE);

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

#endif // PBDRV_CONFIG_UART_STM32_LL_IRQ
