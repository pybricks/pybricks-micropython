#ifndef __NXOS_INTERRUPTS_H__
#define __NXOS_INTERRUPTS_H__

void interrupts_disable();
void interrupts_enable();

/* Default handlers for the three general kinds of interrupts that the
 * ARM core has to handle. These are defined in irq.s, and just freeze
 * the board in an infinite loop.
 */
void default_irq();
void default_fiq();
void spurious_irq();

#endif
