/** @file types.h
 *  @brief Basic type definitions for the Arm7 platform.
 */

/* Copyright (c) 2007,2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_TYPES_H__
#define __NXOS_BASE_TYPES_H__

#include <stddef.h>

/** @addtogroup typesAndUtils */
/*@{*/

#ifndef NULL
/** Definition of the NULL pointer. */
#define NULL ((void*)0)
#endif

/** A function that takes no arguments and returns nothing. */
typedef void (*nx_closure_t)(void);

/*@}*/

#endif
