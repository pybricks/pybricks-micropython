/** @file fs.h
 *  @brief Flash file system
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

#ifndef __NXOS_FS_H__
#define __NXOS_FS_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup fs Flash file system */
/*@{*/

/** Maximum number of files that can be opened in the same time. */
#define FS_MAX_OPENED_FILES 32

/** Maximum allowed filename length. */
#define FS_FILENAME_LENGTH 64

typedef enum {
  FS_ERR_NO_ERROR = 0,
  FS_ERR_FILE_NOT_FOUND,
  FS_ERR_TOO_MANY_OPENED_FILES,
} fs_err_t;

typedef enum {
  FS_PERM_READONLY,
  FS_PERM_READWRITE,
  FS_PERM_EXECUTABLE,
} fs_perm_t;

/** File description structure. */
typedef struct fs_file {
  bool _used;

  char name[FS_FILENAME_LENGTH+1];
  size_t size;
  fs_perm_t perms;
} fs_file_t;

bool nx_fs_init(void);

/** Open a file.
 *
 * @param name The name of the file to open.
 * @param fd A pointer to a U32 where the file descriptor will be stored.
 */
fs_err_t nx_fs_open(char *name, U32 *fd);

/** Close a file. */
void nx_fs_close(U32 fd);

/*@}*/
/*@}*/

#endif /* __NXOS_FS_H__ */
