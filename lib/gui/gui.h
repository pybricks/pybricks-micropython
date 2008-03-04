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

/** Initialize the GUI library. */
void nx_gui_init(void);

// Functions for defining GUI controls should go here.

/** Run the interface main event loop. */
void nx_gui_mainloop(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB_GUI_H__ */

