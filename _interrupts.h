/** @file _interrupts.h
 *  @brief Internal interrupt handling APIs.
 */

/* Copyright (C) 2007 the NxOS developers
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

/** @name Default interrupt handlers.
 *
 * Default handlers for the three general kinds of interrupts that the
 * ARM core has to handle. These are defined in interrupts.S, and just freeze
 * the board in an infinite loop.
 */
/*@{*/
void nx__default_irq();
void nx__default_fiq();
void nx__spurious_irq();
/*@}*/

/*@}*/

#endif /* __NXOS_BASE__INTERRUPTS_H__ */
