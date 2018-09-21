#include <unistd.h>
#include "py/mpconfig.h"
#include "py/runtime.h"
#include "lib/utils/interrupt_char.h"

#include "stm32f070xb.h"

#include "gpio.h"
#include "uartadr.h"

#define RX_BUF_SIZE 32  // must be power of 2!

static uint8_t rx_buf[RX_BUF_SIZE]; 
static volatile uint8_t rx_buf_head;
static volatile uint8_t rx_buf_tail;

void uart_init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_USART4EN | RCC_APB1ENR_USART3EN;

    // enable UART at 115200 baud on BOOST OUT C and D, pin 5 and 6

    // USART 3, BOOST I/O port D
    gpio_init(GPIOB, 0, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    gpio_low(GPIOB, 0);
    gpio_init(GPIOC, 4, GPIO_MODE_ALT, GPIO_PULL_NONE, 1); // USART3_TX
    gpio_init(GPIOC, 5, GPIO_MODE_ALT, GPIO_PULL_NONE, 1); // USART3_RX

    // USART 4, BOOST I/O port C
    gpio_init(GPIOB, 4, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    gpio_low(GPIOB, 4);
    gpio_init(GPIOC, 10, GPIO_MODE_ALT, GPIO_PULL_NONE, 0); // USART4_TX
    gpio_init(GPIOC, 11, GPIO_MODE_ALT, GPIO_PULL_NONE, 0); // USART4_RX

    USART_REPL->BRR = PYBRICKS_SYS_CLOCK_FREQ / 115200;
    USART_REPL->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE;

    NVIC_EnableIRQ(USART3_6_IRQn);
    NVIC_SetPriority(USART3_6_IRQn, 128);
}

void USART3_6_IRQHandler(void) {
    uint8_t c;

    while (USART_REPL->ISR & USART_ISR_RXNE) {
        c = USART_REPL->RDR;
        if (c == mp_interrupt_char) {
            mp_keyboard_interrupt();
        } else {
            uint8_t new_head = (rx_buf_head + 1) & (RX_BUF_SIZE - 1);
            if (new_head == rx_buf_tail) {
                // buffer overrun
                // REVISIT: ignoring for now - will lose characters
                return;
            }
            rx_buf[new_head] = c;
            rx_buf_head = new_head;
        }
    }
}

/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint8_t c;

    // wait for rx interrupt
    while (rx_buf_head == rx_buf_tail) {
        MICROPY_EVENT_POLL_HOOK
    }

    rx_buf_tail = (rx_buf_tail + 1) & (RX_BUF_SIZE - 1);
    c = rx_buf[rx_buf_tail];

    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        // wait for TXE
        while ((USART_REPL->ISR & USART_ISR_TXE) == 0) {
        }
        USART_REPL->TDR = *str++;
    }
}
