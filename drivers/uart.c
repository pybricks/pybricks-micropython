#include "base/at91sam7s256.h"

#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"

#include "base/drivers/uart.h"

static volatile struct {

  uart_read_callback_t callback;
  U32 nmb_int;

  void *buf_a;
  int size_buf_a;
  void *buf_b;
  int size_buf_b;

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



static void uart_isr()
{
  uart_state.nmb_int++;
}



void uart_init(uart_read_callback_t callback,
               void *buf_a, U16 buf_a_size,
               void *buf_b, U16 buf_b_size)
{
  uart_state.callback = callback;

  interrupts_disable();

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

  /* reseting the PDC */

  *AT91C_US1_RPR = NULL;
  *AT91C_US1_RCR = 0;
  *AT91C_US1_TPR = NULL;
  *AT91C_US1_TCR = 0;
  *AT91C_US1_RNPR = NULL;
  *AT91C_US1_RNCR = 0;
  *AT91C_US1_TNPR = NULL;
  *AT91C_US1_TNCR = 0;


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

  *AT91C_US1_CR = AT91C_US_TXEN | AT91C_US_RXEN;


  /* specify the interruptions that this driver needs */

  /* ... */

  /* reenable pio */

  *AT91C_PIOA_PER = UART_PIOA_PINS;

  /*** Interruptions : AIC */
  /* not in edge sensitive mode => level */

  aic_install_isr(AT91C_ID_US1, AIC_PRIO_DRIVER,
                  AIC_TRIG_LEVEL, uart_isr);


  /* reactivate the interruptions */
  interrupts_enable();
}

void uart_write(void *data, U16 lng)
{
  if (*AT91C_US1_TCR == 0) {
    *AT91C_US1_TPR = (U32)data;
    *AT91C_US1_TCR = (U32)lng;

  } else if (*AT91C_US1_TNCR == 0) {

    *AT91C_US1_TNPR = (U32)data;
    *AT91C_US1_TNCR = (U32)lng;

  } else {

    while (*AT91C_US1_TNCR != 0);

    *AT91C_US1_TNPR = (U32)data;
    *AT91C_US1_TNCR = (U32)lng;

  }
}

bool uart_can_write()
{
  return ((*AT91C_US1_TCR == 0) || (*AT91C_US1_TNCR == 0));
}



/****** TO REMOVE : ****/

U32 uart_nmb_interrupt()
{
  return uart_state.nmb_int;
}
