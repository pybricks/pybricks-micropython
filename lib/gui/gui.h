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
#define GUI_EVENT_THROTTLE 450
#define GUI_EVENT_AVOID_REPEAT 300

/** GUI menu description structure. */
typedef struct gui_text_menu {
  /** Menu title. */
  char *title;

  /** Menu entries, NULL terminated array. */
  char **entries;

  /** Number of the default entry. */
  U8 default_entry;

  /** A string replacing '. ' for the currently active entry. */
  char *active_mark;
} gui_text_menu_t;

/** Display the text menu described by @a menu.
 *
 * @param menu A @a gui_text_menu_t structure describing the menu.
 * @return The choosen entry number.
 */
U8 nx_gui_text_menu(gui_text_menu_t menu);


/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB_GUI_H__ */

