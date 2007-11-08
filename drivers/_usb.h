/** @file _usb.h
 *  @brief USB communication internal interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__USB_H__
#define __NXOS_BASE_DRIVERS__USB_H__

#include "base/drivers/usb.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup usbinternal USB communication */
/*@{*/

/** Initialize the USB driver. */
void nx__usb_init();

/** Perform an orderly shutdown of the USB driver.
 *
 * If the brick is connected to a USB bus, it will be properly
 * unregistered.
 */
void nx__usb_disable();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__USB_H__ */
