
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pbdrv/config.h"
#include "pbio/error.h"
#include "pbio/event.h"
#include "pbio/port.h"
#include "pbio/uartdev.h"
#include "pbsys/sys.h"
#include "sys/process.h"

#include "stm32f070xb.h"

#define RX_BUF_SIZE 64  // must be power of 2!

static uint8_t usart4_rx_buf[RX_BUF_SIZE];
static volatile uint8_t usart4_rx_buf_head;
static uint8_t usart4_rx_buf_tail;

static uint8_t usart3_rx_buf[RX_BUF_SIZE];
static volatile uint8_t usart3_rx_buf_head;
static uint8_t usart3_rx_buf_tail;

PROCESS(pbdrv_uart_process, "UART");

static pbio_error_t _pbdrv_uart_get_char(pbio_port_t port, uint8_t *c, bool peek) {
    switch (port) {
    case PBIO_PORT_C:
        if (usart4_rx_buf_head == usart4_rx_buf_tail) {
            return PBIO_ERROR_AGAIN;
        }

        *c = usart4_rx_buf[usart4_rx_buf_tail];
        if (!peek) {
            usart4_rx_buf_tail = (usart4_rx_buf_tail + 1) & (RX_BUF_SIZE - 1);
        }
        break;
    case PBIO_PORT_D:
        if (usart3_rx_buf_head == usart3_rx_buf_tail) {
            return PBIO_ERROR_AGAIN;
        }

        *c = usart3_rx_buf[usart3_rx_buf_tail];
        if (!peek) {
            usart3_rx_buf_tail = (usart3_rx_buf_tail + 1) & (RX_BUF_SIZE - 1);
        }
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_peek_char(pbio_port_t port, uint8_t *c) {
    return _pbdrv_uart_get_char(port, c, true);
}

pbio_error_t pbdrv_uart_get_char(pbio_port_t port, uint8_t *c) {
    return _pbdrv_uart_get_char(port, c, false);
}

pbio_error_t pbdrv_uart_put_char(pbio_port_t port, uint8_t c) {
    switch (port) {
    case PBIO_PORT_C:
        if (!(USART4->ISR & USART_ISR_TXE)) {
            return PBIO_ERROR_AGAIN;
        }
        USART4->TDR = c;
        while (!(USART4->ISR & USART_ISR_TC)) { }
        USART4->ICR |= USART_ICR_TCCF;
        break;
    case PBIO_PORT_D:
        if (!(USART3->ISR & USART_ISR_TXE)) {
            return PBIO_ERROR_AGAIN;
        }
        USART3->TDR = c;
        while (!(USART3->ISR & USART_ISR_TC)) { }
        USART3->ICR |= USART_ICR_TCCF;
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_uart_set_baud_rate(pbio_port_t port, uint32_t baud) {
    switch (port) {
    case PBIO_PORT_C:
        USART4->BRR = PBDRV_CONFIG_SYS_CLOCK_RATE / baud;
        break;
    case PBIO_PORT_D:
        USART3->BRR = PBDRV_CONFIG_SYS_CLOCK_RATE / baud;
        break;
    default:
        return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}

static void uart_init() {
    // enable power domains
    RCC->APB1ENR |= RCC_APB1ENR_USART4EN | RCC_APB1ENR_USART3EN;

    // note: pin mux is handled in ioport.c

    // enable the UARTs
    USART4->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;

    pbdrv_uart_set_baud_rate(PBIO_PORT_C, 2400);
    pbdrv_uart_set_baud_rate(PBIO_PORT_D, 2400);

    // DMA is not possible on USART3/4 on STM32F070x6, so using interrupt
    NVIC_EnableIRQ(USART3_6_IRQn);
    NVIC_SetPriority(USART3_6_IRQn, 128);
}

// overrides weak function in setup_*.m
void USART3_6_IRQHandler(void) {
    uint8_t c, new_head;

    // port C
    while (USART4->ISR & USART_ISR_RXNE) {
        c = USART4->RDR;

        new_head = (usart4_rx_buf_head + 1) & (RX_BUF_SIZE - 1);
        if (new_head == usart4_rx_buf_tail) {
            // buffer overrun
            // REVISIT: ignoring for now - will lose characters
            continue;
        }
        usart4_rx_buf[usart4_rx_buf_head] = c;
        usart4_rx_buf_head = new_head;
    }

    // port D
    while (USART3->ISR & USART_ISR_RXNE) {
        c = USART3->RDR;
        new_head = (usart3_rx_buf_head + 1) & (RX_BUF_SIZE - 1);
        if (new_head == usart3_rx_buf_tail) {
            // buffer overrun
            // REVISIT: ignoring for now - will lose characters
            continue;
        }
        usart3_rx_buf[usart3_rx_buf_head] = c;
        usart3_rx_buf_head = new_head;
    }

    process_poll(&pbdrv_uart_process);
}

static void handle_poll() {
    uint8_t c;

    while (pbdrv_uart_get_char(PBIO_PORT_C, &c) == PBIO_SUCCESS) {
        pbio_event_uart_rx_data_t rx = { .port = PBIO_PORT_C, .byte = c };
        process_post_synch(&pbio_uartdev_process, PBIO_EVENT_UART_RX, rx.data);
    }
    while (pbdrv_uart_get_char(PBIO_PORT_D, &c) == PBIO_SUCCESS) {
        pbio_event_uart_rx_data_t rx = { .port = PBIO_PORT_D, .byte = c };
        process_post_synch(&pbio_uartdev_process, PBIO_EVENT_UART_RX, rx.data);
    }
}

PROCESS_THREAD(pbdrv_uart_process, ev, data) {
    PROCESS_POLLHANDLER(handle_poll());

    PROCESS_BEGIN();

    uart_init();

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}
