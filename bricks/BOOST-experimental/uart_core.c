#include <unistd.h>
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */

#if MICROPY_MIN_USE_STM32_MCU
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
#include "uartadr.h"
#endif

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
#if MICROPY_MIN_USE_STDOUT
    int r = read(0, &c, 1);
    (void)r;
#elif MICROPY_MIN_USE_STM32_MCU
    // wait for RXNE
    while ((USART_REPL->USART_ISR & (1 << 5)) == 0) {
    }
    c = USART_REPL->RDR;
#endif
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
#if MICROPY_MIN_USE_STDOUT
    int r = write(1, str, len);
    (void)r;
#elif MICROPY_MIN_USE_STM32_MCU
    while (len--) {
        // wait for TXE
        while ((USART_REPL->USART_ISR & (1 << 7)) == 0) {
        }
        USART_REPL->TDR = *str++;
    }
#endif
}
