#ifndef __NXOS_INTERRUPTS_H__
#define __NXOS_INTERRUPTS_H__

#include "base/types.h"

void nx_interrupts_disable();
void nx_interrupts_enable();

/* The following structure describes the order in which registers are
 * pushed on the system stack when the IRQ handler interrupts a
 * user/system mode task. If you map a pointer to this structure on the
 * stack pointer during an interrupt, you will get all the registers
 * mapped properly in the struct.
 *
 * Yes, the structure is messy and disorganized, but the order is a
 * direct result of the order in which the IRQ handler has to push
 * things.
 *
 * This is not used in the base code, but is very useful if you want to
 * implement a scheduler that pokes around in task states.
 */
typedef struct {
  U32 cpsr;
  U32 pc;
  U32 r0;
  U32 r1;
  U32 r2;
  U32 r3;
  U32 r4;
  U32 r5;
  U32 r6;
  U32 r7;
  U32 r8;
  U32 r9;
  U32 r10;
  U32 r11;
  U32 r12;
  U32 lr;
} nx_task_stack_t;

#endif
