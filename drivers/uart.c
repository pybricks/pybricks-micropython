#include "base/at91sam7s256.h"

#include "base/mytypes.h"

#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"

#include "base/drivers/uart.h"

#include "base/drivers/systick.h"
#include "base/display.h"


static volatile struct {

  uart_read_callback_t callback;
  U32 nmb_int;

  /* read buffers */
  void *buf_a_ptr;
  U16 buf_a_size;
  void *buf_b_ptr;
  U16 buf_b_size;
  bool current_buf;

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
  U32 csr;

  csr = *AT91C_US1_CSR;

  if (csr & AT91C_US_ENDRX) {

    if (!uart_state.current_buf) {
      *AT91C_US1_RNPR = (U32)uart_state.buf_a_ptr;
      *AT91C_US1_RNCR = uart_state.buf_a_size;
    } else {
      *AT91C_US1_RNPR = (U32)uart_state.buf_b_ptr;
      *AT91C_US1_RNCR = uart_state.buf_b_size;
    }

    uart_state.current_buf = !uart_state.current_buf;
  }

  uart_state.nmb_int++;
}



void uart_init(uart_read_callback_t callback,
               void *buf_a, U16 buf_a_size,
               void *buf_b, U16 buf_b_size)
{
  uart_state.callback = callback;
  uart_state.buf_a_ptr = buf_a;
  uart_state.buf_b_ptr = buf_b;
  uart_state.buf_a_size = buf_a_size;
  uart_state.buf_b_size = buf_b_size;

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

  /**  then reset everything */

  /* disable all the interruptions */

  *AT91C_US1_IDR = ~0;

  /* configure/reset the PDC */

  *AT91C_US1_RPR = (U32)buf_a;
  *AT91C_US1_RCR = buf_a_size;
  *AT91C_US1_TPR = NULL;
  *AT91C_US1_TCR = 0;
  *AT91C_US1_RNPR = (U32)buf_b;
  *AT91C_US1_RNCR = buf_b_size;
  *AT91C_US1_TNPR = NULL;
  *AT91C_US1_TNCR = 0;


  /* then configure: */


  /* configure the mode register */
  *AT91C_US1_MR =
    AT91C_US_USMODE_HWHSH /* hardware handshaking */
    |AT91C_US_CLKS_CLOCK /* MCK */
    |AT91C_US_CHRL_8_BITS
    /* no sync */
    |AT91C_US_NBSTOP_1_BIT
    |AT91C_US_PAR_NONE
    |AT91C_US_CHMODE_NORMAL /* normal mode (no test mode) */
    |AT91C_US_OVER; /* oversampling : 8x */


  /* and then the baud rate generator: */
  /*   we want to transmit at 460.8Kbauds */
  /*   we selected the master clock, with an oversampling of 8x */
  /*   => So Baud rate = MCK / (Clock Divisor * 8) */
  /*   => Clock Divisor = MCK / 8 / Baud rate  */
#define UART_BAUD_RATE 460800
#define UART_CLOCK_DIVISOR(mck, baudrate) (mck / 8 / baudrate)

  *AT91C_US1_BRGR = UART_CLOCK_DIVISOR(NXT_CLOCK_FREQ, UART_BAUD_RATE)
    | ( (((NXT_CLOCK_FREQ/8) - (UART_CLOCK_DIVISOR(NXT_CLOCK_FREQ, UART_BAUD_RATE) * UART_BAUD_RATE)) / ((UART_BAUD_RATE + 4)/8)) << 16);

#undef UART_CLOCK_DIVISOR

  /* Reset everything in the control register */

  *AT91C_US1_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA | AT91C_US_RSTNACK;


  /* specify the interruptions that this driver needs */

  *AT91C_US1_IER = AT91C_US_ENDRX;

  /* reenable pio */

  *AT91C_PIOA_PER = UART_PIOA_PINS;
  *AT91C_PIOA_OER = UART_PIOA_PINS;

  /*** Interruptions : AIC */
  /* not in edge sensitive mode => level */

  aic_install_isr(AT91C_ID_US1, AIC_PRIO_DRIVER,
                  AIC_TRIG_LEVEL, uart_isr);


   /* and then reenable the transmitter and the receiver thanks to the
   * TXEN bit and the RXEN bit in US_CR */

  *AT91C_US1_CR = AT91C_US_TXEN | AT91C_US_RXEN;

  /* enable the pdc transmitter / receiver */

  *AT91C_US1_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN;

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

  systick_wait_ms(1000);

  display_clear();

  display_hex(*AT91C_US1_CSR);


  systick_wait_ms(2000);
}

bool uart_can_write()
{
  return ((*AT91C_US1_TCR == 0) || (*AT91C_US1_TNCR == 0));
}



/****** TO REMOVE : ****/

U32 uart_writing()
{
  return (*AT91C_US1_TCR + *AT91C_US1_TNCR);
}

U32 uart_nmb_interrupt()
{
  return uart_state.nmb_int;
}
