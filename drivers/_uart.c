/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"
#include "base/drivers/systick.h"

#include "base/drivers/_uart.h"

#define UART_BUFSIZE 128

static volatile struct {
  uart_read_callback_t callback;
  U32 packet_size;

  /* read buffers */
  U8 buf[UART_BUFSIZE];
  U32 state;

  /* to remove */
  U32 last_csr;
  U32 nmb_int;
} uart_state = {
  0
};

/* I/O (uart 1)
 * RXD1 => PA21 (periph A)
 * TXD1 => PA22 (periph A)
 * CTS1 => PA25 (periph A)
 * RTS1 => PA24 (periph A)
 */
#define UART_PIOA_PINS \
   (AT91C_PA21_RXD1 |  \
    AT91C_PA22_TXD1 |  \
    AT91C_PA25_CTS1 |  \
    AT91C_PA24_RTS1 |  \
    AT91C_PA23_SCK1)

static void uart_isr() {
  U32 csr;

  uart_state.nmb_int++;


  csr = *AT91C_US1_CSR;

  uart_state.last_csr = csr;

  if (csr & AT91C_US_RXBRK) {
    uart_state.callback(NULL, 0);
    *AT91C_US1_CR = AT91C_US_RSTSTA;
  }

  if (csr & AT91C_US_RXRDY) {

    /* we've just read the first byte:
     * it's the packet size */
    /* now we will use the PDC to read the whole packet */

    *AT91C_US1_IDR = AT91C_US_RXRDY;
    while(*AT91C_US1_IMR & AT91C_US_RXRDY);

    uart_state.packet_size = *AT91C_US1_RHR & 0xFF;
    *AT91C_US1_RCR = uart_state.packet_size;

    /* we reenable the receiving with the PDC */

    *AT91C_US1_IER = AT91C_US_ENDRX;
    *AT91C_US1_PTCR = AT91C_PDC_RXTEN;
  }

  if (csr & AT91C_US_ENDRX) {
    *AT91C_US1_PTCR = AT91C_PDC_RXTDIS;

    uart_state.callback((U8*)&(uart_state.buf), uart_state.packet_size);

    /* We must put a size != 0 in the RCR register (even if the PDC is disabled for the receiving) */
    /* else when we try to read manually a value on US1_RHR thanks to the RXRDY interruption
     * the RXRDY of the CSR seems to never be set to 1 (no value read on the UART ?) */
    /* TODO : figure this out */
    *AT91C_US1_RPR = (U32)(&uart_state.buf);
    *AT91C_US1_RCR = UART_BUFSIZE; /* default size */

    /* we've read a packet, so now we will do a manual reading
     * to have the next packet size and adapt the PDC RCR register value */
    *AT91C_US1_IER = AT91C_US_RXRDY;
  }
}

void nx_uart_init(uart_read_callback_t callback) {
  uart_state.callback = callback;

  nx_interrupts_disable();

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

  /* Reset everything in the control register */

  *AT91C_US1_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RSTSTA | AT91C_US_RSTNACK;

  /* configure/reset the PDC */

  /* We must put a size != 0 in the RCR register (even if the PDC is disabled for the receiving) */
  /* else when we try to read manually a value on US1_RHR thanks to the RXRDY interruption
   * the RXRDY of the CSR seems to never be set to 1 (no value read on the UART ?) */
  /* TODO : figure this out */
  *AT91C_US1_RPR = (U32)(&uart_state.buf);
  *AT91C_US1_RCR = UART_BUFSIZE;
  *AT91C_US1_TPR = 0;
  *AT91C_US1_TCR = 0;
  *AT91C_US1_RNPR = 0;
  *AT91C_US1_RNCR = 0;
  *AT91C_US1_TNPR = 0;
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
  //*AT91C_US1_BRGR = UART_CLOCK_DIVISOR(NXT_CLOCK_FREQ, UART_BAUD_RATE);

#undef UART_CLOCK_DIVISOR

  *AT91C_US1_CR = AT91C_US_STTTO;


  /* specify the interruptions that this driver needs */

  *AT91C_US1_IER = AT91C_US_RXRDY | AT91C_US_RXBRK;

  /*** Interruptions : AIC */
  /* not in edge sensitive mode => level */

  nx_aic_install_isr(AT91C_ID_US1, AIC_PRIO_DRIVER,
		     AIC_TRIG_LEVEL, uart_isr);


   /* and then reenable the transmitter and the receiver thanks to the
    * TXEN bit and the RXEN bit in US_CR */

  *AT91C_US1_CR = AT91C_US_TXEN | AT91C_US_RXEN;

  /* enable the pdc transmitter */

  *AT91C_US1_PTCR = AT91C_PDC_TXTEN;

  /* reactivate the interruptions */
  nx_interrupts_enable();

}

void nx_uart_write(void *data, U32 lng) {
  while (*AT91C_US1_TNCR != 0);

  *AT91C_US1_TNPR = (U32)data;
  *AT91C_US1_TNCR = lng;
}

bool nx_uart_can_write() {
  return (*AT91C_US1_TNCR == 0);
}

bool nx_uart_is_writing() {
  return (*AT91C_US1_TCR + *AT91C_US1_TNCR) > 0;
}

/****** TO REMOVE : ****/

U32 nx_uart_nmb_interrupt()
{
  return uart_state.nmb_int;
}

U32 nx_uart_get_last_csr()
{
  return uart_state.last_csr;
}

U32 nx_uart_get_csr()
{
  return *AT91C_US1_CSR;
}

U32 nx_uart_get_state()
{
  return uart_state.packet_size;
}
