
#include <pbsys/sys.h>
#include <pbdrv/uart.h>

#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint8_t c;

    // wait for rx interrupt
    while (pbsys_stdin_get_char(&c) != PBIO_SUCCESS) {
        MICROPY_EVENT_POLL_HOOK
    }

    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    uint8_t c;

    while (len--) {
        c = *str++;
        // busy-wait until char has been sent
        while (pbdrv_uart_put_char(PBIO_PORT_C, c) != PBIO_SUCCESS) { }
    }
}
