/** @file _fs.h
 *  @brief Flash file system internal APIs.
 *
 * A flash-friendly file system for the NXT on-board flash memory.
 */

/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE__FS_H__
#define __NXOS_BASE__FS_H__

#include "base/types.h"
#include "fs.h"

/** @addtogroup kernelinternal */
/*@{*/

/** @name File system internals.
 *
 */
/*@{*/

volatile fs_file_t fdset[FS_MAX_OPENED_FILES];

/** Returns a file info structure given its file descriptor.
 *
 * @param fd The file descriptor (must be valid, as in pointing to
 * a opened file).
 * @return The file info structure if the @a fd is valid, or NULL
 * if it refers to a no longer opened or inexistant file.
 */
volatile fs_file_t *nx_fs_get_file(fs_fd_t fd);

/** Determines if the given page contains a file origin marker.
 *
 * @param page The page number.
 */
inline bool nx_fs_page_has_magic(U32 page);

/** Find a file's origin on the file system by its name.
 *
 * @param name The file name.
 * @param origin A pointer to an U32 where the result will be stored.
 * @return An appropriate @a fs_err_t error code.
 */
fs_err_t nx__fs_find_file_origin(char *name, U32 *origin);

/*@}*/
/*@}*/


#endif /* __NXOS_BASE__FS_H__ */
