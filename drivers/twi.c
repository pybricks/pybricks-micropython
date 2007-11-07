
#include "base/types.h"
#include "base/drivers/_twi.h"
#include "base/interrupts.h"
#include "base/at91sam7s256.h"

#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/lock.h"

static volatile enum {
  TWI_UNINITIALISED = 0,
  TWI_FAILED,
  TWI_READY,
  TWI_TX_BUSY,
  TWI_RX_BUSY,
} twi_state = TWI_UNINITIALISED;

static volatile struct {
  U32 len;
  U8 *ptr;
} twi_request = { 0, NULL };


static void
twi_isr()
{
  U32 status = *AT91C_TWI_SR;

  if ((status & AT91C_TWI_RXRDY) && twi_state == TWI_RX_BUSY) {
    if (twi_request.len) {
      *twi_request.ptr = *AT91C_TWI_RHR;
      twi_request.ptr++;
      twi_request.len--;

      if (twi_request.len == 1) {
	*AT91C_TWI_CR = AT91C_TWI_STOP;
      }
      if (twi_request.len == 0) {
        *AT91C_TWI_IDR = ~0;
	twi_state = TWI_READY;
      }
    }
  }

  if ((status & AT91C_TWI_TXRDY) && twi_state == TWI_TX_BUSY) {
    if (twi_request.len) {
      *AT91C_TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_START;
      if (twi_request.len == 1) {
	*AT91C_TWI_CR = AT91C_TWI_STOP;
      }
      *AT91C_TWI_THR = *twi_request.ptr;

      twi_request.ptr++;
      twi_request.len--;
    } else {
      *AT91C_TWI_IDR = ~0;
      twi_state = TWI_READY;
    }
  }

  if (status & (AT91C_TWI_OVRE | AT91C_TWI_UNRE)) {
    *AT91C_TWI_CR = AT91C_TWI_STOP;
    *AT91C_TWI_IDR = ~0;
    twi_state = TWI_FAILED;
  }

  if (status & AT91C_TWI_NACK) {
    *AT91C_TWI_CR = AT91C_TWI_STOP;
    *AT91C_TWI_IDR = ~0;
    twi_state = TWI_FAILED;
  }
}


bool nx__twi_ready() {
  return (twi_state == TWI_READY) ? TRUE : FALSE;
}

void nx__twi_init()
{
  U32 clocks = 9;

  nx_interrupts_disable();

  *AT91C_PMC_PCER = ((1 << AT91C_ID_PIOA) |  /* Need PIO too */
                     (1 << AT91C_ID_TWI));   /* TWI clock domain */

  *AT91C_TWI_IDR = ~0;

  /* Set up pin as an IO pin for clocking till clean */
  *AT91C_PIOA_MDER = (1 << 3) | (1 << 4);
  *AT91C_PIOA_PER = (1 << 3) | (1 << 4);
  *AT91C_PIOA_ODR = (1 << 3);
  *AT91C_PIOA_OER = (1 << 4);

  while (clocks > 0 && !(*AT91C_PIOA_PDSR & (1 << 3))) {
    *AT91C_PIOA_CODR = (1 << 4);
    nx_systick_wait_ns(1500);
    *AT91C_PIOA_SODR = (1 << 4);
    nx_systick_wait_ns(1500);
    clocks--;
  }

  *AT91C_PIOA_PDR = (1 << 3) | (1 << 4);
  *AT91C_PIOA_ASR = (1 << 3) | (1 << 4);

  *AT91C_TWI_CR = 0x88;		/* Disable & reset */

  *AT91C_TWI_CWGR = 0x020f0f;	/* Set for 380kHz */
  *AT91C_TWI_CR = 0x04;		/* Enable as master */

  twi_state = TWI_READY;

  nx_aic_install_isr(AT91C_ID_TWI, AIC_PRIO_RT,
		     AIC_TRIG_LEVEL, twi_isr);

  nx_interrupts_enable();
}

void nx__twi_read_async(U32 dev_addr, U8 *data, U32 nBytes)
{
  U32 mode = ((dev_addr & 0x7f) << 16) | AT91C_TWI_IADRSZ_NO | AT91C_TWI_MREAD;
  U32 dummy;

  twi_state = TWI_RX_BUSY;
  *AT91C_TWI_IDR = ~0;	/* Disable all interrupts */

  twi_request.ptr = data;
  twi_request.len = nBytes;

  dummy = *AT91C_TWI_SR;
  dummy = *AT91C_TWI_RHR;

  *AT91C_TWI_MMR = mode;
  *AT91C_TWI_CR = AT91C_TWI_START | AT91C_TWI_MSEN;

  /* Tell the TWI to send an interrupt when a byte is received, or
   * when there is a NAK (error) condition. */
  *AT91C_TWI_IER = AT91C_TWI_RXRDY;
}

void nx__twi_write_async(U32 dev_addr, U8 *data, U32 nBytes)
{
  U32 mode = ((dev_addr & 0x7f) << 16) | AT91C_TWI_IADRSZ_NO;

  twi_state = TWI_TX_BUSY;
  *AT91C_TWI_IDR = ~0;	/* Disable all interrupts */

  twi_request.ptr = data;
  twi_request.len = nBytes;

  *AT91C_TWI_MMR = mode;
  *AT91C_TWI_CR = AT91C_TWI_START | AT91C_TWI_MSEN;
  *AT91C_TWI_IER = AT91C_TWI_TXRDY;
}
