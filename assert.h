/** @file assert.h
 *  @brief Assertion and crash control.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_ASSERT_H__
#define __NXOS_BASE_ASSERT_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup assert Assertions
 *
 * Error control on an embedded system is a rather hard task. Because of
 * this special environment, returning a nice <tt>EINVAL</tt> when
 * preconditions fail isn't really useful.
 *
 * The Baseplate provides macros that allow the kernel to check that
 * preconditions hold. If the precondition fails, an assertion failure
 * report is printed to the display and the kernel hangs. The brick can
 * be powered down by pressing the cancel button.
 *
 * @note For all assertion macros, if the assertion fails, the statement
 * never returns.
 *
 * @note Remember that the assertion will display on the NXT's
 * screen. You have only about 3 16 character lines to display your
 * message, so you'll have to be terse.
 *
 * @warning The assertion code still makes use of kernel drivers to
 * display the assertion error. Do not verify assertions when interrupt
 * handling is disabled.
 *
 * @warning Your assertion expressions should not have side-effects. In
 * the future, assertions will be deactivatable, so their absence should
 * not change your execution flow (other than not failing for invalid
 * preconditions, obviously).
 */
/*@{*/

/** @cond DOXYGEN_SKIP */
void nx_assert_error(const char *file, const int line,
		     const char *expr, const char *msg);
/** @endcond */

/** Check that @a expr holds true, and assert with @a msg if not.
 *
 * @param expr A boolean expression to check.
 * @param msg The message to display if the assertion triggers.
 */
#define NX_ASSERT_MSG(expr, msg) do {      \
  bool _result = (expr);                   \
  if (_result == FALSE) {                  \
    nx_assert_error(__FILE__, __LINE__,    \
                    "(" #expr ")", msg);   \
  }                                        \
} while(0)

/** Check that @a expr holds true.
 *
 * Similar to NX_ASSERT_MSG(), but a standard "Assertion failed" message
 * will be displayed.
 *
 * @param expr A boolean expression to check.
 */
#define NX_ASSERT(expr) NX_ASSERT_MSG(expr, "Assertion failed")

/** Trigger an assertion error with @a msg.
 *
 * Use this in cases where you want to manually trigger the assertion
 * error code after checking more complex preconditions.
 *
 * @param msg The message to display if the assertion triggers.
 */
#define NX_FAIL(msg) nx_assert_error(__FILE__, __LINE__, msg, "")

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_ASSERT_H__ */
