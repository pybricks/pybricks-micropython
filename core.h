/** @file core.h
 *  @brief Core NxOS startup and shutdown APIs.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_CORE_H__
#define __NXOS_BASE_CORE_H__

/** @addtogroup kernel */
/*@{*/

/* @defgroup core Core startup and shutdown.
 *
 * Since most of the baseplate's bootup procedure is internal, there
 * isn't much to see here, just a few things of use to application
 * kernels to gracefully handle shutdown.
 */
/*@{*/

/** Perform an orderly shutdown of the NXT brick.
 *
 * This function is the only safe way to power down the brick, as it
 * performs a few operations before actually powering off, such as
 * placing the LCD display in a safe state and cleanly unregistering
 * from a USB chain, if the brick was plugged in.
 */
void nx_core_halt();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_CORE_H__ */
