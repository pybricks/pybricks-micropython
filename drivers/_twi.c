/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"
#include "base/drivers/systick.h"

#include "base/drivers/_twi.h"

enum twi_mode {
    TWI_UNINITIALIZED = 0,
    TWI_FAILED,
    TWI_READY,
    TWI_TX_BUSY,
    TWI_RX_BUSY,
};

static volatile struct {
  /* The current state of the TWI driver state machine. */
  enum twi_mode mode;

  /* Address and size of a send/receive buffer. */
  U8 *ptr;
  U32 len;
} twi_state = {
  TWI_UNINITIALIZED, /* Not initialized yet. */
  NULL,              /* No send/recv buffer. */
  0,                 /* And zero length, obviously. */
};

static void twi_isr()
{
  /* Read the status register once to acknowledge all TWI interrupts. */
  U32 status = *AT91C_TWI_SR;

  /* Read mode and the status indicates a byte was received. */
  if (twi_state.mode == TWI_RX_BUSY && (status & AT91C_TWI_RXRDY)) {
    *twi_state.ptr = *AT91C_TWI_RHR;
    twi_state.ptr++;
    twi_state.len--;

    /* If only one byte is left to read, instruct the TWI to send a STOP
     * condition after the next byte.
     */
    if (twi_state.len == 1) {
      *AT91C_TWI_CR = AT91C_TWI_STOP;
    }

    /* If the read is over, inhibit all TWI interrupts and return to the
     * ready state.
     */
    if (twi_state.len == 0) {
      *AT91C_TWI_IDR = ~0;
      twi_state.mode = TWI_READY;
    }
  }

  /* Write mode and the status indicated a byte was transmitted. */
  if (twi_state.mode == TWI_TX_BUSY && (status & AT91C_TWI_TXRDY)) {
    /* If that was the last byte, inhibit TWI interrupts and return to
     * the ready state.
     */
    if (twi_state.len == 0) {
      *AT91C_TWI_IDR = ~0;
      twi_state.mode = TWI_READY;
    } else {
      /* Instruct the TWI to send a STOP condition at the end of the
       * next byte if this is the last byte.
       */
      if (twi_state.len == 1)
	*AT91C_TWI_CR = AT91C_TWI_STOP;

      /* Write the next byte to the transmit register. */
      *AT91C_TWI_THR = *twi_state.ptr;
      twi_state.ptr++;
      twi_state.len--;
    }
  }

  /* Check for error conditions. There are pretty critical failures,
   * since they indicate something is very wrong with either this
   * driver, or the coprocessor, or the hardware link between them.
   */
  if (status & (AT91C_TWI_OVRE | AT91C_TWI_UNRE | AT91C_TWI_NACK)) {
    *AT91C_TWI_CR = AT91C_TWI_STOP;
    *AT91C_TWI_IDR = ~0;
    twi_state.mode = TWI_FAILED;
    /* TODO: This should be an assertion failed, ie. a hard crash. */
  }
}

void nx__twi_init()
{
  U32 clocks = 9;

  nx_interrupts_disable();

  /* Power up the TWI and PIO controllers. */
  *AT91C_PMC_PCER = (1 << AT91C_ID_TWI) | (1 << AT91C_ID_PIOA);

  /* Inhibit all TWI interrupt sources. */
  *AT91C_TWI_IDR = ~0;

  /* If the system is rebooting, the coprocessor might believe that it
   * is in the middle of an I2C transaction. Furthermore, it may be
   * pulling the data line low, in the case of a read transaction.
   *
   * The TWI hardware has a bug that will lock it up if it is
   * initialized when one of the clock or data lines is low.
   *
   * So, before initializing the TWI, we manually take control of the
   * pins using the PIO controller, and manually drive a few clock
   * cycles, until the data line goes high.
   */
  *AT91C_PIOA_MDER = AT91C_PA3_TWD | AT91C_PA4_TWCK;
  *AT91C_PIOA_PER = AT91C_PA3_TWD | AT91C_PA4_TWCK;
  *AT91C_PIOA_ODR = AT91C_PA3_TWD;
  *AT91C_PIOA_OER = AT91C_PA4_TWCK;

  while (clocks > 0 && !(*AT91C_PIOA_PDSR & AT91C_PA3_TWD)) {
    *AT91C_PIOA_CODR = AT91C_PA4_TWCK;
    nx_systick_wait_ns(1500);
    *AT91C_PIOA_SODR = AT91C_PA4_TWCK;
    nx_systick_wait_ns(1500);
    clocks--;
  }

  nx_interrupts_enable();

  /* Now that the I2C lines are clean, hand them back to the TWI
   * controller.
   */
  *AT91C_PIOA_PDR = AT91C_PA3_TWD | AT91C_PA4_TWCK;
  *AT91C_PIOA_ASR = AT91C_PA3_TWD | AT91C_PA4_TWCK;

  /* Reset the controller and configure its clock. The clock setting
   * makes the TWI run at 380kHz.
   */
  *AT91C_TWI_CR = AT91C_TWI_SWRST | AT91C_TWI_MSDIS;
  *AT91C_TWI_CWGR = 0x020f0f;
  *AT91C_TWI_CR = AT91C_TWI_MSEN;
  twi_state.mode = TWI_READY;

  /* Install the TWI interrupt handler. */
  nx_aic_install_isr(AT91C_ID_TWI, AIC_PRIO_RT, AIC_TRIG_LEVEL, twi_isr);
}

void nx__twi_read_async(U32 dev_addr, U8 *data, U32 len)
{
  U32 mode = ((dev_addr & 0x7f) << 16) | AT91C_TWI_IADRSZ_NO | AT91C_TWI_MREAD;

  /* TODO: assert(nx__twi_ready()) */

  twi_state.mode = TWI_RX_BUSY;
  twi_state.ptr = data;
  twi_state.len = len;

  *AT91C_TWI_MMR = mode;
  *AT91C_TWI_CR = AT91C_TWI_START | AT91C_TWI_MSEN;
  *AT91C_TWI_IER = AT91C_TWI_RXRDY;
}

void nx__twi_write_async(U32 dev_addr, U8 *data, U32 len)
{
  U32 mode = ((dev_addr & 0x7f) << 16) | AT91C_TWI_IADRSZ_NO;

  /* TODO: assert(nx__twi_ready()) */

  twi_state.mode = TWI_TX_BUSY;
  twi_state.ptr = data;
  twi_state.len = len;

  *AT91C_TWI_MMR = mode;
  *AT91C_TWI_CR = AT91C_TWI_START | AT91C_TWI_MSEN;
  *AT91C_TWI_IER = AT91C_TWI_TXRDY;
}

bool nx__twi_ready() {
  return (twi_state.mode == TWI_READY) ? TRUE : FALSE;
}
