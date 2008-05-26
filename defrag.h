/** @file defrag.h
 *  @brief Flash file system defragmentation utilities.
 *
 * A defragmentation and optimization tool for the NxOS file system.
 */

/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DEFRAG_H__
#define __NXOS_BASE_DEFRAG_H__

#include "base/types.h"
#include "base/drivers/_efc.h"
#include "_fs.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup defragmentation Defragmentation utilities */
/*@{*/

typedef enum {
  DEFRAG_ERR_NO_ERROR,
  DEFRAG_ERR_NOT_ENOUGH_SPACE,
  DEFRAG_ERR_FLASH_ERROR,
} defrag_err_t;

/** Perform a simple defragmentation of the flash filesystem.
 *
 * This type of defragmentation only tries to collate files towards
 * the beginning of the flash medium, maximizing free space at the end of
 * the flash.
 *
 * @return A @a defrag_err_t describing the outcome of the operation.
 */
defrag_err_t nx_defrag_simple(void);

/** Perform a simple, file oriented defragmentation of the flash.
 *
 * This defragmentation type is similar to the simple one, but
 * with the objective of making subsequent writes to the given file
 * (by its name) faster by putting it at the end of the flash medium.
 *
 * @return A @a defrag_err_t describing the outcome of the operation.
 */
defrag_err_t nx_defrag_for_file_by_name(char *name);

/** Same as @a nx_defrag_for_file_by_name but takes a file origin
 * instead. This function is called by @a nx_defrag_for_file_by_name
 * once it has found the requested file's origin.
 *
 * @return A @a defrag_err_t describing the outcome of the operation.
 */
defrag_err_t nx_defrag_for_file_by_origin(U32 origin);

/** Tries to optimize the placement of the files on the filesystem
 * to make write operations faster for all files by putting as much
 * space as possible between each file and thus avoid a costy file
 * relocation.
 *
 * @return A @a defrag_err_t describing the outcome of the operation.
 */
defrag_err_t nx_defrag_best_overall(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DEFRAG_H__ */
