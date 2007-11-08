/** @file _lcd.h
 *  @brief LCD controller interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__LCD_H__
#define __NXOS_BASE_DRIVERS__LCD_H__

#include "base/types.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup lcdinternal LCD controller
 *
 * The LCD controller drives the UC1601 display chip, which in turn
 * drives the refresh cycle of the LCD display.
 */
/*@{*/

/** Width of the LCD display, in pixels. */
#define LCD_WIDTH 100
/** Height of the LCD display, in bytes. */
#define LCD_HEIGHT 8 /* == 64 pixels. */

/** Initialize the LCD driver. */
void nx__lcd_init();

/** Periodic update function, called once every millisecond.
 *
 * @warning This is called by the systick driver, and shouldn't be
 * invoked directly unless you really know what you are doing.
 */
void nx__lcd_fast_update();

/** Set the virtual display to mirror to the screen.
 *
 * @param display_buffer The screen buffer to mirror.
 */
void nx__lcd_set_display(U8 *display_buffer);

/** Mark the display as requiring a refresh cycle. */
void nx__lcd_dirty_display();

/** Safely power off the LCD controller.
 *
 * The LCD controller must be powered off this way in order to drain
 * several capacitors connected to the display. Failure to do so may
 * damage the LCD display (although in practice the screen seems to
 * take hard poweroffs fairly happily).
 */
void nx__lcd_shutdown();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__LCD_H__ */
