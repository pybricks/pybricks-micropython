/** @file _core.h
 *  @brief NxOS core internal APIs.
 *
 * Technically, this file isn't even needed, since only assembler code
 * refers to the baseplate's main routine. However, to keep the compiler
 * happy about predeclaring functions, and to be future proof, this file
 * exists.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE__CORE_H__
#define __NXOS_BASE__CORE_H__

#include "base/core.h"

/** @addtogroup kernelinternal */
/*@{*/

/** @defgroup core Core startup and shutdown
 *
 * The baseplate's main() is private.
 */
/*@{*/

/** Initialize the baseplate and run the application payload.
 *
 */
void nx__kernel_main(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE__CORE_H__ */
