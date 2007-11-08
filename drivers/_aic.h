/** @file _aic.h
 *  @brief Interrupt controller internal interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__AIC_H__
#define __NXOS_BASE_DRIVERS__AIC_H__

#include "base/drivers/aic.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup aicinternal Interrupt controller */
/*@{*/

/** Initialize the interrupt controller. */
void nx__aic_init();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__AIC_H__ */
