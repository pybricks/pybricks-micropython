/** @file _motors.h
 *  @brief Motor control internal interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__MOTORS_H__
#define __NXOS_BASE_DRIVERS__MOTORS_H__

#include "base/drivers/motors.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup motorsinternal Motor control */
/*@{*/

/** Initialize the motor control driver. */
void nx__motors_init(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__MOTORS_H__ */
