/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 *
 */

#include "base/at91sam7s256.h"

#include "base/nxt.h"
#include "base/assert.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"

#include "base/drivers/rs485.h"

static inline U16 build_baud_rate(nx_rs485_baudrate_t br) {
  return NXT_CLOCK_FREQ / 16 / br;
}

static volatile struct {
  enum {
    RS485_UNINITIALIZED = 0,
    RS485_IDLE,
    RS485_TRANSMITTING,
    RS485_RECEIVING
  } status; /* Status of the device driver */
  void (*callback)(nx_rs485_error_t); /* End of operation callback */
} rs485_state;

static inline U32 build_usart_modreg (U32 x) {
  return AT91C_US_USMODE_RS485 | x;
}

/* This constant represents the set of communication channel to be used for
 * rs485 transmission/reception
 * - PA5 Line of Receive data
 * - PA6 Line of Transmit data
 * - PA7 Line of Ready to send
 */
static const U32 usart_to_rs485_lines = AT91C_PIO_PA5 |
                                        AT91C_PIO_PA6 |
                                        AT91C_PIO_PA7;

static inline void fire_callback(nx_rs485_error_t status, bool reset) {
  if (reset) {
    *AT91C_US0_CR = AT91C_US_RSTSTA;
  }
  if (rs485_state.callback) {
    rs485_state.callback(status);
  }
}

static void nx_rs485_isr(void) {
  const U32 csr = *AT91C_US0_CSR;

  /* Successful transfer completion first, then error cases. */
  if ((rs485_state.status == RS485_RECEIVING && (csr & AT91C_US_ENDRX)) ||
      (rs485_state.status == RS485_TRANSMITTING && (csr & AT91C_US_ENDTX))) {
    *AT91C_US0_IDR = AT91C_US_ENDRX | AT91C_US_ENDTX;
    *AT91C_US0_CR = AT91C_US_RXDIS | AT91C_US_TXDIS;
    rs485_state.status = RS485_IDLE;
    fire_callback(RS485_SUCCESS, FALSE);
  } else if (csr & AT91C_US_TIMEOUT) {
    *AT91C_US0_CR = AT91C_US_STTTO;
    fire_callback(RS485_TIMEOUT, FALSE);
  } else if (csr & AT91C_US_OVRE) {
    fire_callback(RS485_OVERRUN, TRUE);
  } else if (csr & AT91C_US_FRAME) {
    fire_callback(RS485_FRAMING, TRUE);
  } else if (csr & AT91C_US_PARE) {
    fire_callback(RS485_PARITY, TRUE);
  }
}

void nx_rs485_init(nx_rs485_baudrate_t baud_rate,
                   U32 uart_mr,
                   U16 timeout,
                   bool timeguard) {
  NX_ASSERT(rs485_state.status == RS485_UNINITIALIZED);

  /* Enable the peripheral clock */
  *AT91C_PMC_PCER = (1 << AT91C_ID_US0);

  /* Disable the PIOA controller pin management */
  *AT91C_PIOA_PPUDR = usart_to_rs485_lines;
  *AT91C_PIOA_PDR   = usart_to_rs485_lines;
  *AT91C_PIOA_ASR   = usart_to_rs485_lines;

  /* Reset and disable to avoid funny jokes: */
  *AT91C_US0_CR = AT91C_US_RSTRX | /* Transmitter reset */
                  AT91C_US_RSTTX | /* Receiver reset */
                  AT91C_US_RSTSTA; /* Status bits */

  /* Placing the Usart settings in the appropriate register, time guard,
   * receiver timeout and baud rate */
  *AT91C_US0_MR = uart_mr == 0 ? build_usart_modreg(DEFAULT_USART_MODEREG)
                               : build_usart_modreg(uart_mr);
  *AT91C_US0_TTGR = timeguard;
  *AT91C_US0_RTOR = timeout;
  *AT91C_US0_BRGR = build_baud_rate(baud_rate);

  /* Enable USART interrupts for error conditions. Normal transmission
   * conditions are enabled only when transfers are initiated.
   */
  *AT91C_US0_IDR = ~0;
  *AT91C_US0_IER = AT91C_US_OVRE | /* Overrun error (overwritten data) */
                   AT91C_US_FRAME; /* Framing error */
  if (timeout > 0) {
    *AT91C_US0_IER = AT91C_US_TIMEOUT; /* Timeout */
  }
  if ((uart_mr & AT91C_US_PAR) != AT91C_US_PAR_NONE) {
    *AT91C_US0_IER = AT91C_US_PARE; /* Parity error */
  }

  /* Install the rs485 ISR. */
  nx_aic_install_isr(AT91C_ID_US0, AIC_PRIO_DRIVER, AIC_TRIG_LEVEL,
                     nx_rs485_isr);

  rs485_state.status = RS485_IDLE;
}

bool nx_rs485_set_fixed_baudrate(U16 baud_rate) {
  if (rs485_state.status == RS485_IDLE) {
    *AT91C_US0_BRGR = baud_rate;
    return TRUE;
  }
  return FALSE;
}

void nx_rs485_shutdown(void) {
  NX_ASSERT(rs485_state.status == RS485_IDLE);

  *AT91C_US0_CR = AT91C_US_RSTTX | /* Transmitter reset */
                  AT91C_US_RSTRX | /* Receiver reset */
                  AT91C_US_RSTSTA; /* Status reset */

  /* Disable usart clock */
  *AT91C_PMC_PCDR = (1 << AT91C_ID_US0);

  rs485_state.status = RS485_UNINITIALIZED;
}

bool nx_rs485_send(U8 *buffer, U32 buflen, void (*callback)(nx_rs485_error_t)) {
  NX_ASSERT(buflen > 0);
  NX_ASSERT(rs485_state.status != RS485_UNINITIALIZED);
  if (rs485_state.status != RS485_IDLE)
    return FALSE;

  rs485_state.status = RS485_TRANSMITTING;
  rs485_state.callback = callback;

  /* Set up a DMA write and start transferring asynchronously. */
  *AT91C_US0_TPR = (U32)buffer;
  *AT91C_US0_TCR = buflen;
  *AT91C_US0_IER = AT91C_US_ENDTX;
  *AT91C_US0_PTCR = AT91C_PDC_TXTEN;
  *AT91C_US0_CR = AT91C_US_TXEN;

  return TRUE;
}

bool nx_rs485_recv(U8 *buffer, U32 buflen, void (*callback)(nx_rs485_error_t)) {
  NX_ASSERT(buflen > 0);
  NX_ASSERT(rs485_state.status != RS485_UNINITIALIZED);
  if (rs485_state.status != RS485_IDLE)
    return FALSE;

  rs485_state.status = RS485_RECEIVING;
  rs485_state.callback = callback;

  /* Set up a DMA read and start transferring asynchronously. */
  *AT91C_US0_RPR = (U32)buffer;
  *AT91C_US0_RCR = buflen;
  *AT91C_US0_IER = AT91C_US_ENDRX;
  *AT91C_US0_PTCR = AT91C_PDC_RXTEN;

  /* Due to (apparently) the wiring of the RS485 bus, both the
   * transmitter and the receiver have to be enabled, otherwise the
   * receiver does not receive any data.
   *
   * TODO: Figure out the exact reason and document it.
   */
  *AT91C_US0_CR = AT91C_US_RXEN | AT91C_US_TXEN;

  return TRUE;
}

void nx_rs485_abort(void) {
  NX_ASSERT(rs485_state.status != RS485_UNINITIALIZED);
  if (rs485_state.status == RS485_IDLE)
    return;

  /* Stop the communication and reset */
  *AT91C_US0_CR = AT91C_US_RSTRX | AT91C_US_RSTTX;
  rs485_state.status = RS485_IDLE;
  fire_callback(RS485_ABORT, TRUE);
}

