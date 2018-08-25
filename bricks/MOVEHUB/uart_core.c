#include <unistd.h>
#include "py/mpconfig.h"
#include "uartadr.h"

/*
 * Core UART functions to implement for a port
 */

typedef struct {
    volatile uint32_t CR1;
    uint32_t d[2];
    volatile uint32_t BRR;
    uint32_t e[3];
    volatile uint32_t USART_ISR;
    uint32_t f[1];
    volatile uint32_t RDR;
    volatile uint32_t TDR;
} periph_uart_t;

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;

    // wait for RXNE
    while ((USART_REPL->USART_ISR & (1 << 5)) == 0) {
    }
    c = USART_REPL->RDR;

    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        // wait for TXE
        while ((USART_REPL->USART_ISR & (1 << 7)) == 0) {
        }
        USART_REPL->TDR = *str++;
    }
}
