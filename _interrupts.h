#ifndef __NXOS__INTERRUPTS_H__
#define __NXOS__INTERRUPTS_H__

#include "base/interrupts.h"

/* Default handlers for the three general kinds of interrupts that the
 * ARM core has to handle. These are defined in irq.s, and just freeze
 * the board in an infinite loop.
 */
void nx_default_irq();
void nx_default_fiq();
void nx_spurious_irq();

#endif
