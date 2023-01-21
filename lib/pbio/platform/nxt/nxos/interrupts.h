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

#ifndef __NXOS_BASE_INTERRUPTS_H__
#define __NXOS_BASE_INTERRUPTS_H__

#include <stdint.h>

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
void nx_interrupts_disable(void);

/** Enable interrupt handling.
 *
 * Interrupt handling will only be reenabled if this function has been
 * called the same number of times as nx_interrupts_disable().
 */
void nx_interrupts_enable(void);

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
  uint32_t cpsr; /**< CPU status register. */
  uint32_t pc; /**< Program Counter register. */
  uint32_t r0; /**< General Purpose Register 0. */
  uint32_t r1; /**< General Purpose Register 1. */
  uint32_t r2; /**< General Purpose Register 2. */
  uint32_t r3; /**< General Purpose Register 3. */
  uint32_t r4; /**< General Purpose Register 4. */
  uint32_t r5; /**< General Purpose Register 5. */
  uint32_t r6; /**< General Purpose Register 6. */
  uint32_t r7; /**< General Purpose Register 7. */
  uint32_t r8; /**< General Purpose Register 8. */
  uint32_t r9; /**< General Purpose Register 9. */
  uint32_t r10; /**< General Purpose Register 10. */
  uint32_t r11; /**< General Purpose Register 11. */
  uint32_t r12; /**< General Purpose Register 12. */
  uint32_t lr; /**< Link Register. */
} nx_task_stack_t;

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_INTERRUPTS_H__ */
