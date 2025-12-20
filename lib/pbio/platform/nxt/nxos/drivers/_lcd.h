/** @file _lcd.h
 *  @brief LCD controller interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__LCD_H__
#define __NXOS_BASE_DRIVERS__LCD_H__

#include <stdint.h>

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup lcdinternal LCD controller
 *
 * This driver contains a basic SPI driver to talk to the UltraChip
 * 1601 LCD controller, as well as a higher level API implementing the
 * UC1601's commandset.
 *
 * Note that the SPI driver is not suitable as a general-purpose SPI
 * driver: the MISO pin (Master-In Slave-Out) is instead wired to the
 * UC1601's CD input (used to select whether the transferred data is
 * control commands or display data). Thus, the SPI driver here takes
 * manual control of the MISO pin, and drives it depending on the type
 * of data being transferred.
 *
 * This also means that you can only write to the UC1601, not read
 * back from it. This is not too much of a problem, as we can just
 * introduce a little delay in the places where we really need it.
 */
/*@{*/

/** Width of the LCD display, in pixels. */
#define LCD_WIDTH 100
/** Height of the LCD display, in bytes. */
#define LCD_HEIGHT 8 /* == 64 pixels. */

/** Display an abort message.
 *
 * This will take the kernel offline (the technical term for "crash")
 * after displaying an abort message.
 */
void nx__lcd_sync_refresh(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__LCD_H__ */
