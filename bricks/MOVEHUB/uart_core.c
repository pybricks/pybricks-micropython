#include <unistd.h>
#include "py/mpconfig.h"

#include "stm32f030xc.h"

#include "uartadr.h"

/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;

    // wait for RXNE
    while ((USART_REPL->ISR & USART_ISR_RXNE) == 0) {
    }
    c = USART_REPL->RDR;

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
