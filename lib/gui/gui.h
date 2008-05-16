/** @file gui.h
 *  @brief Graphical user interface library.
 *
 * A GUI library for NxOS.
 */

/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LIB_GUI_H__
#define __NXOS_BASE_LIB_GUI_H__

#include "base/types.h"

/** @addtogroup lib */
/*@{*/

/** @defgroup gui Graphical user interface library
 *
 * This GUI library provides an easy-to-use interface for creating
 * and using a GUI displayed to the user.
 */
/*@{*/

/** Wait time in milliseconds before next event polling. */
#define GUI_EVENT_THROTTLE 350

/** Wait time in milliseconds after a menu entry was selected
 * to avoid stroke repeat.
 */
#define GUI_EVENT_AVOID_REPEAT 300

/** Default menu marker for active entry. */
#define GUI_DEFAULT_TEXT_MARK "> "

/** GUI menu description structure. */
typedef struct gui_text_menu {
  char *title;       /**< Menu title. */
  char **entries;    /**< Menu entries, NULL terminated array. */
  U8 default_entry;  /** Number of the default entry. */
  char *active_mark; /**< A string replacing '. ' for the
                      * currently active entry. */
} gui_text_menu_t;

/** Display the text menu described by @a menu.
 *
 * @param menu A @a gui_text_menu_t structure describing the menu.
 * @return The choosen entry number.
 */
U8 nx_gui_text_menu(gui_text_menu_t menu);

/** Display a yes/no text menu.
 *
 * @param title The menu title.
 * @return True for 'yes', false for 'no'.
 */
bool nx_gui_text_menu_yesno(char *title);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB_GUI_H__ */

