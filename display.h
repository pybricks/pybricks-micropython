/** @file display.h
 *  @brief Display handling interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DISPLAY_H__
#define __NXOS_BASE_DISPLAY_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup display Display
 *
 * The shenanigans of handling refreshes of the LCD display are neatly
 * abstracted away by NxOS. The display is an abstract sheet of memory,
 * which can be poked using various functions to display things.
 *
 * One important part of the abstraction is the cursor. It is similar to
 * ncurse's notion of a cursor: it is where the next character will be
 * printed. The cursor is initially in the top-left corner of the
 * screen, will move automatically as you write things to the screen,
 * and automatically wraps when you hit either the right or bottom edge
 * of the screen.
 *
 * The screen is initially in auto-refresh mode: any time you write
 * anything to the screen, the physical LCD is automatically refreshed.
 * Auto-refresh can be disabled, in which case the physical screen will
 * only refresh itself on an explicit call to nx_display_refresh().
 */
/*@{*/

/** Clear the display and reset the cursor to (0,0). */
void nx_display_clear(void);

/** Set the refreshing mode of the display.
 *
 * @param auto_refresh TRUE if the display should be refreshed
 * implicitely when written to, FALSE if you want to give explicit
 * refresh commands.
 */
void nx_display_auto_refresh(bool auto_refresh);

/** Start a display refresh cycle.
 *
 * @note This call has very little effect if the display is in
 * auto-refresh mode: a refresh cycle will be triggered, but since the
 * display is being refreshed anyway, you probably won't notice a
 * difference.
 */
inline void nx_display_refresh(void);

/** Move the cursor to line @a x and column @a y.
 *
 * @param x The cursor's new line position.
 * @param y The cursor's new column position.
 *
 * @note Both @a x and @a y are zero-indexed: (0,0) is the top-left
 * corner of the display.
 */
void nx_display_cursor_set_pos(U8 x, U8 y);

/** Print a single line feed. */
inline void nx_display_end_line(void);

/** Display @a str at the current cursor position.
 *
 * @param str The string to display.
 *
 * @note Any unprintable characters in the string are rendered as
 * spaces.
 */
void nx_display_string(const char *str);

/** Display @a val as a hexadecimal number.
 *
 * @param val The number to display in hex.
 */
void nx_display_hex(U32 val);

/** Display @a val as a decimal number.
 *
 * @param val The number to display.
 */
void nx_display_uint(U32 val);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DISPLAY_H__ */
