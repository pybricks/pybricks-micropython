/** @file _abort.h
 *  @brief Abort handler API.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE__ABORT_H__
#define __NXOS_BASE__ABORT_H__

void nx__abort(bool data, U32 pc, U32 cpsr);

#endif /* __NXOS_BASE__ABORT_H__ */
