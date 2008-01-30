/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/_interrupts.h"

#include "base/drivers/_aic.h"

void nx__aic_init(void) {
  int i;

  /* Prevent the ARM core from being interrupted while we set up the
   * AIC.
   */
  nx_interrupts_disable();

  /* If we're coming from a warm boot, the AIC may be in a weird
   * state. Do some cleaning up to bring the AIC back into a known
   * state:
   *  - All interrupt lines disabled,
   *  - No interrupt lines handled by the FIQ handler,
   *  - No pending interrupts,
   *  - AIC idle, not handling an interrupt.
   */
  *AT91C_AIC_IDCR = 0xFFFFFFFF;
  *AT91C_AIC_FFDR = 0xFFFFFFFF;
  *AT91C_AIC_ICCR = 0xFFFFFFFF;
  *AT91C_AIC_EOICR = 1;

  /* Enable debug protection. This is necessary for JTAG debugging, so
   * that the hardware debugger can read AIC registers without
   * triggering side-effects.
   */
  *AT91C_AIC_DCR = 1;

  /* Set default handlers for all interrupt lines. */
  for (i = 0; i < 32; i++) {
    AT91C_AIC_SMR[i] = 0;
    AT91C_AIC_SVR[i] = (U32) nx__default_irq;
  }
  AT91C_AIC_SVR[AT91C_ID_FIQ] = (U32) nx__default_fiq;
  *AT91C_AIC_SPU = (U32) nx__spurious_irq;

  nx_interrupts_enable();
}

void nx_aic_install_isr(nx_aic_vector_t vector, nx_aic_priority_t prio,
                     nx_aic_trigger_mode_t trig_mode, nx_closure_t isr) {
  /* Disable the interrupt we're installing. Getting interrupted while
   * we are tweaking it could be bad.
   */
  nx_aic_disable(vector);
  nx_aic_clear(vector);

  AT91C_AIC_SMR[vector] = (trig_mode << 5) | prio;
  AT91C_AIC_SVR[vector] = (U32)isr;

  nx_aic_enable(vector);
}

void nx_aic_enable(nx_aic_vector_t vector) {
  *AT91C_AIC_IECR = (1 << vector);
}

void nx_aic_disable(nx_aic_vector_t vector) {
  *AT91C_AIC_IDCR = (1 << vector);
}

void nx_aic_set(nx_aic_vector_t vector) {
  *AT91C_AIC_ISCR = (1 << vector);
}

void nx_aic_clear(nx_aic_vector_t vector) {
  *AT91C_AIC_ICCR = (1 << vector);
}
