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
#include "base/assert.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"
#include "base/drivers/systick.h"

#include "base/drivers/_uart.h"

/* Buffer size for UART messages. */
#define UART_BUFSIZE 128

/* Pinmask for all the UART pins. */
#define UART_PIOA_PINS \
   (AT91C_PA21_RXD1 |  \
    AT91C_PA22_TXD1 |  \
    AT91C_PA25_CTS1 |  \
    AT91C_PA24_RTS1 |  \
    AT91C_PA23_SCK1)

/* UART baud rate: 460.8kBaud */
#define UART_BAUD_RATE 460800

/* Value of the UART Clock Divisor to get as close as possible to that
 * value. This divisor actually programs for 461.5kBaud, for 0.15%
 * error.
 */
#define UART_CLOCK_DIVISOR (NXT_CLOCK_FREQ / 8 / UART_BAUD_RATE)

static volatile struct {
  nx__uart_read_callback_t callback;

  U32 packet_size;
  U8 buf[UART_BUFSIZE];
} uart_state = {
  NULL, 0, {0}
};

static void uart_isr() {
  U32 status = *AT91C_US1_CSR;

  /* If we receive a break condition from the Bluecore, send up a NULL
   * packet and reset the controller status.
   */
  if (status & AT91C_US_RXBRK) {
    uart_state.callback(NULL, 0);
    *AT91C_US1_CR = AT91C_US_RSTSTA;
  }

  if (status & AT91C_US_RXRDY) {

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

  if (status & AT91C_US_ENDRX) {
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

void nx__uart_init(nx__uart_read_callback_t callback) {
  uart_state.callback = callback;

  nx_interrupts_disable();

  /* Power up the USART. */
  *AT91C_PMC_PCER = (1 << AT91C_ID_US1);

  /* Hand the USART I/O pins over to the controller. */
  *AT91C_PIOA_PDR = UART_PIOA_PINS;
  *AT91C_PIOA_ASR = UART_PIOA_PINS;

  /* Disable both receiver and transmitter, inhibit USART interrupts,
   * and reset all of the controller's components.
   */
  *AT91C_US1_CR = AT91C_US_TXDIS | AT91C_US_RXDIS;
  *AT91C_US1_IDR = ~0;
  *AT91C_US1_CR = (AT91C_US_RSTRX | AT91C_US_RSTTX |
		   AT91C_US_RSTSTA | AT91C_US_RSTNACK);

  /* configure/reset the PDC */

  /* We must put a size != 0 in the RCR register (even if the PDC is
   * disabled for the receiving) else when we try to read manually a
   * value on US1_RHR thanks to the RXRDY interruption the RXRDY of the
   * CSR seems to never be set to 1 (no value read on the UART ?)
   *
   * TODO : figure this out
   */
  *AT91C_US1_RPR = (U32)(&uart_state.buf);
  *AT91C_US1_RCR = UART_BUFSIZE;
  *AT91C_US1_TPR = 0;
  *AT91C_US1_TCR = 0;
  *AT91C_US1_RNPR = 0;
  *AT91C_US1_RNCR = 0;
  *AT91C_US1_TNPR = 0;
  *AT91C_US1_TNCR = 0;

  /* Configure the USART for:
   *  - Hardware handshaking
   *  - Master clock as the clock source
   *  - 8-bit characters, 1 stop bit, no parity bit
   *  - Asynchronous communication
   *  - No receive timeout
   */
  *AT91C_US1_MR = (AT91C_US_USMODE_HWHSH | AT91C_US_CLKS_CLOCK |
		   AT91C_US_CHRL_8_BITS | AT91C_US_NBSTOP_1_BIT |
		   AT91C_US_PAR_NONE | AT91C_US_CHMODE_NORMAL |
		   AT91C_US_OVER);
  *AT91C_US1_BRGR = UART_CLOCK_DIVISOR;
  *AT91C_US1_RTOR = 0;

  /* Start listening for a character to receive. Since we programmed no
   * timeout, this will just start listening until something happens.
   */
  *AT91C_US1_CR = AT91C_US_STTTO;

  /* Install an interrupt handler and start listening on interesting
   * interrupt sources.
   */
  nx_aic_install_isr(AT91C_ID_US1, AIC_PRIO_DRIVER,
		     AIC_TRIG_LEVEL, uart_isr);
  *AT91C_US1_IER = AT91C_US_RXRDY | AT91C_US_RXBRK;

  /* Activate the USART and its associated DMA controller. */
  *AT91C_US1_CR = AT91C_US_TXEN | AT91C_US_RXEN;
  *AT91C_US1_PTCR = AT91C_PDC_TXTEN;

  nx_interrupts_enable();
}

void nx__uart_write(const U8 *data, U32 lng) {
  NX_ASSERT(data != NULL);
  NX_ASSERT(lng > 0);

  while (*AT91C_US1_TNCR != 0);

  *AT91C_US1_TNPR = (U32)data;
  *AT91C_US1_TNCR = lng;
}

bool nx__uart_can_write() {
  return (*AT91C_US1_TNCR == 0);
}

bool nx__uart_is_writing() {
  return (*AT91C_US1_TCR + *AT91C_US1_TNCR) > 0;
}
