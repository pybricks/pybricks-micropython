#include "base/at91sam7s256.h"

#include "base/nxt.h"
#include "base/drivers/aic.h"

#include "base/drivers/uart.h"

static volatile struct {

  uart_read_callback_t callback;

} uart_state = {
  0
};




/*** I/O (uart 1) */
/* SCK1 => PA23 (periph A)
 * TXD1 => PA22 (periph A)
 * RI1  => PA29 (periph A)
 * DSR1 => PA28 (periph A)
 * DCD1 => PA26 (periph A)
 * DTR1 => PA27 (periph A)
 * CTS1 => PA25 (periph A)
 * RTS1 => PA24 (periph A)
 */
#define UART_PIOA_PINS \
  (AT91C_PA23_SCK1 \
   | AT91C_PA22_TXD1 \
   | AT91C_PA29_RI1 \
   | AT91C_PA28_DSR1 \
   | AT91C_PA26_DCD1 \
   | AT91C_PA27_DTR1 \
   | AT91C_PA25_CTS1 \
   | AT91C_PA24_RTS1)


void uart_init(uart_read_callback_t callback)
{
  uart_state.callback = callback;


  /* pio : we disable pio management
   * and then switch to the periph A (uart) */
  *AT91C_PIOA_PDR = UART_PIOA_PINS;
  *AT91C_PIOA_ASR = UART_PIOA_PINS;


  /*** clock & power : PMC */
  /* must enable the USART clock (USART 1)*/

  *AT91C_PMC_PCER = (1 << AT91C_ID_US1);

  /*** configuration : USART registers */

  /* first of all : disable the transmitter and the receiver
   * thanks to the TXDIS and RXDIS in US_CR */

  *AT91C_US1_CR = AT91C_US_TXDIS | AT91C_US_RXDIS;

  /* then reset everything
   * to reset : set RSTRX and RSTTX in the US_CR */

  *AT91C_US1_CR = AT91C_US_RSTRX | AT91C_US_RSTTX;

  /* then configure: */

  *AT91C_US1_MR =
    AT91C_US_USMODE_HWHSH /* hardware handshaking */
    |AT91C_US_CLKS_CLOCK
    |AT91C_US_CHRL_8_BITS
    /* no sync */
    |AT91C_US_NBSTOP_1_BIT
    |AT91C_US_PAR_NONE
    |AT91C_US_OVER; /* oversampling : 8x */

  /* and then reenable the transmitter and the receiver thanks to the
   * TXEN bit and the RXEN bit in US_CR */


  /* reenable pio */


  /*** Interruptions : AIC */
  /* not in edge sensitive mode => level */



  /*** Interruptions : PDC */


}

void uart_write(void *data, U16 lng)
{

}

bool uart_can_write()
{
  return 0;
}
