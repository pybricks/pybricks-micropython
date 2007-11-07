/** @file _systick.h
 *  @brief System timer internal interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__SYSTICK_H__
#define __NXOS_BASE_DRIVERS__SYSTICK_H__

#include "base/drivers/systick.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup systickinternal System timer */
/*@{*/

/** Initialize the system timer driver. */
void nx__systick_init();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__SYSTICK_H__ */
