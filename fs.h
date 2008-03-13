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
#include "base/drivers/_efc.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup fs Flash file system */
/*@{*/

/** Maximum number of files that can be stored by the filesystem.
 * The lack of dynamic memory allocator makes this a hardcoded
 * limitation.
 */
#define FS_MAX_OPENED_FILES 8

/** Maximum allowed filename length. */
#define FS_FILENAME_LENGTH 64

/** File I/O operations buffer size. */
#define FS_BUF_SIZE (EFC_PAGE_WORDS * sizeof(U32))

/** File system errors. */
typedef enum {
  FS_ERR_NO_ERROR = 0,
  FS_ERR_NOT_FORMATTED,
  FS_ERR_FILE_NOT_FOUND,
  FS_ERR_TOO_MANY_OPENED_FILES,
  FS_ERR_INVALID_FD,
} fs_err_t;

/** File permissions. */
typedef enum {
  FS_PERM_READONLY,
  FS_PERM_READWRITE,
  FS_PERM_EXECUTABLE,
} fs_perm_t;

/** File description structure. */
typedef struct {
  char name[FS_FILENAME_LENGTH+1];
  size_t size;
  fs_perm_t perms;
  bool _used;

  int *rpos;

  U32 wbuf[FS_BUF_SIZE];
  U16 wpos;
} fs_file_t;

typedef U8 fs_fd_t;

fs_err_t nx_fs_init(void);

/** Open a file.
 *
 * @param name The name of the file to open.
 * @param fd A pointer to the file descriptor to use.
 */
fs_err_t nx_fs_open(char *name, fs_fd_t *fd);

/** Get the file size.
 *
 * @param fs The file descriptor.
 * @return The file size as a @a size_t.
 */
size_t nx_fs_get_filesize(fs_fd_t fd);

/** Read one byte from a file.
 *
 * @param fd The descriptor for the file to read from.
 * @param byte A pointer to a U32 to write the read byte from.
 */
fs_err_t nx_fs_read(fs_fd_t fd, U32 *byte);

/** Write one byte to a file.
 *
 * @param fd The descriptor for the file to write to.
 * @param byte The byte to write to the file.
 * @return An fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_write(fs_fd_t fd, U32 byte);

/** Flush a file's write buffer. */
fs_err_t nx_fs_flush(fs_fd_t fd);

/** Close a file. */
fs_err_t nx_fs_close(fs_fd_t fd);

/*@}*/
/*@}*/

#endif /* __NXOS_FS_H__ */
