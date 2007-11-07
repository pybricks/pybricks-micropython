/** @file _display.h
 *  @brief Internal display APIs.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */
#ifndef __NXOS_BASE__DISPLAY_H__
#define __NXOS_BASE__DISPLAY_H__

#include "base/types.h"
#include "display.h"

/** @addtogroup kernelinternal */
/*@{*/

/** @name Display internals.
 *
 * The display's initialization function is private, since the baseplate
 * is the only one that should be able to initialize it.
 */
/*@{*/

/** Initialize the display driver. */
void nx__display_init();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE__DISPLAY_H__ */
