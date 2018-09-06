#include <unistd.h>
#include "py/mpconfig.h"
#include "py/runtime.h"
#include "lib/utils/interrupt_char.h"

#include "stm32f070xb.h"

#include "uartadr.h"

#define RX_BUF_SIZE 32  // must be power of 2!

static uint8_t rx_buf[RX_BUF_SIZE]; 
static volatile uint8_t rx_buf_head;
static volatile uint8_t rx_buf_tail;

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
