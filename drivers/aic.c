/* Driver for the AT91SAM7's Advanced Interrupt Controller (AIC).
 *
 * The AIC is responsible for queuing interrupts from other
 * peripherals on the board. It then hands them one by one to the ARM
 * CPU core for handling, according to each peripheral's configured
 * priority.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/_interrupts.h"
#include "base/drivers/aic.h"

/* Initialise the Advanced Interrupt Controller.
 *
 * Note that this function leaves interrupts disabled in the ARM core
 * when it returns.
 */
void nx__aic_init() {
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


/* Register an interrupt service routine for an interrupt line.
 *
 * Note that while this function registers the routine in the AIC, it
 * does not enable or disable the interrupt line for that vector. Use
 * aic_mask_on and aic_mask_off to control actual activation of the
 * interrupt line.
 *
 * Args:
 *   vector: The peripheral ID to claim (see AT91SAM7.h for peripheral IDs)
 *   mode: The priority of this interrupt in relation to others. See aic.h
 *         for a list of defined values.
 *   isr: A pointer to the interrupt service routine function.
 */
void nx_aic_install_isr(aic_vector_t vector, aic_priority_t prio,
                     aic_trigger_mode_t trig_mode, aic_isr_t isr) {
  /* Disable the interrupt we're installing. Getting interrupted while
   * we are tweaking it could be bad.
   */
  nx_aic_disable(vector);
  nx_aic_clear(vector);

  AT91C_AIC_SMR[vector] = (trig_mode << 5) | prio;
  AT91C_AIC_SVR[vector] = (U32)isr;

  nx_aic_enable(vector);
}


/* Enable handling of an interrupt line in the AIC.
 *
 * Args:
 *   vector: The peripheral ID of the interrupt line to enable.
 */
void nx_aic_enable(aic_vector_t vector) {
  *AT91C_AIC_IECR = (1 << vector);
}


/* Disable handling of an interrupt line in the AIC.
 *
 * Args:
 *   vector: The peripheral ID of the interrupt line to disable.
 */
void nx_aic_disable(aic_vector_t vector) {
  *AT91C_AIC_IDCR = (1 << vector);
}


/* Set an interrupt line in the AIC.
 *
 * Args:
 *   vector: The peripheral ID of the interrupt line to set.
 */
void nx_aic_set(aic_vector_t vector) {
  *AT91C_AIC_ISCR = (1 << vector);
}


/* Clear an interrupt line in the AIC.
 *
 * Args:
 *   vector: The peripheral ID of the interrupt line to clear.
 */
void nx_aic_clear(aic_vector_t vector) {
  *AT91C_AIC_ICCR = (1 << vector);
}
