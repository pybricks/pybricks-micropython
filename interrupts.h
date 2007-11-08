/** @file interrupts.h
 *  @brief Interrupts and task information.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_INTERRUPTS_H__
#define __NXOS_INTERRUPTS_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup interrupt Interrupts and task information
 *
 * The Baseplate provides facilities for enabling and disabling
 * interrupts, and for obtaining information about an interrupted user
 * task.
 *
 * The Baseplate itself provides no scheduler, but its interrupt
 * dispatch routine is tailored so that, when it interrupts code running
 * in User/System mode, all the state is saved to the User/System mode
 * stack. This makes the User/System mode task entirely self-contained
 * within its stack pointer, which can be exchanged trivially for
 * another to implement simple task switching.
 */
/*@{*/

/** Globally disable interrupt handling.
 *
 * This function call can be nested. Internally, a counter counts the
 * number of disables, and will require the same number of calls to
 * nx_interrupts_enable() to reenable interrupt handling.
 *
 * @note Application kernels enter the main() function with interrupts
 * already enabled.
 *
 * @warning The NXT cannot function for more than about a millisecond
 * with interrupts disabled. Disabling them for too long will cause the
 * coprocessor link to fail, which will bring the whole system crashing
 * down. Use sparingly, for small critical sections.
 *
 * @sa nx_interrupts_enable
 */
void nx_interrupts_disable();

/** Enable interrupt handling.
 *
 * Interrupt handling will only be reenabled if this function has been
 * called the same number of times as nx_interrupts_disable().
 */
void nx_interrupts_enable();

/** @brief The mapping of a user task's registers in the User/System stack.
 *
 * This structure should be used in an interrupt handler: cast the
 * User/System stack pointer to a pointer to this structure, and
 * dereference the fields to access the values of registers.
 *
 * @note This structure only works for user tasks. It does not
 * accurately describe the content of any stack when there is no user
 * mode task running.
 */
typedef struct {
  U32 cpsr; /**< CPU status register. */
  U32 pc; /**< Program Counter register. */
  U32 r0; /**< General Purpose Register 0. */
  U32 r1; /**< General Purpose Register 1. */
  U32 r2; /**< General Purpose Register 2. */
  U32 r3; /**< General Purpose Register 3. */
  U32 r4; /**< General Purpose Register 4. */
  U32 r5; /**< General Purpose Register 5. */
  U32 r6; /**< General Purpose Register 6. */
  U32 r7; /**< General Purpose Register 7. */
  U32 r8; /**< General Purpose Register 8. */
  U32 r9; /**< General Purpose Register 9. */
  U32 r10; /**< General Purpose Register 10. */
  U32 r11; /**< General Purpose Register 11. */
  U32 r12; /**< General Purpose Register 12. */
  U32 lr; /**< Link Register. */
} nx_task_stack_t;

/*@}*/
/*@}*/

#endif
