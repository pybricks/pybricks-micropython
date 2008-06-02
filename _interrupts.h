/** @file _interrupts.h
 *  @brief Internal interrupt handling APIs.
 */

/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE__INTERRUPTS_H__
#define __NXOS_BASE__INTERRUPTS_H__

#include "base/interrupts.h"

/** @addtogroup kernelinternal */
/*@{*/

/** @defgroup interruptinternal Interrupts and task information
 *
 * This module just contains internal handlers related to exception
 * handling.
 */
/*@{*/

/** @name Default interrupt handlers.
 *
 * Default handlers for the three general kinds of interrupts that the
 * ARM core has to handle. These are defined in interrupts.S, and just freeze
 * the board in an infinite loop.
 */
/*@{*/
void nx__default_irq(void);
void nx__default_fiq(void);
void nx__spurious_irq(void);
/*@}*/

/*@}*/
/*@}*/

#endif /* __NXOS_BASE__INTERRUPTS_H__ */
