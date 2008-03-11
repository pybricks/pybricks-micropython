/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/assert.h"
#include "base/util.h"
#include "base/fs.h"
#include "base/drivers/_efc.h"

static volatile fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Initialize the file system, most importantly check for file system
 * integrity. */
fs_err_t nx_fs_init(void) {
  return FS_ERR_NO_ERROR;
}

/* Open or create a file by its name. The associated file descriptor
 * is returned via the fd pointer argument.
 */
fs_err_t nx_fs_open(char *name, fs_fd_t *fd) {
  U32 name_len = strlen(name);

  NX_ASSERT(name_len > 0 && name_len < FS_FILENAME_LENGTH);

  *fd = -1;

  return FS_ERR_FILE_NOT_FOUND;
}

static volatile fs_file_t *nx_fs_get_file(fs_fd_t fd) {
  NX_ASSERT(fd > 0);
  NX_ASSERT(fd < FS_MAX_OPENED_FILES);

  return &(fdset[fd]);
}

/* Read one byte from the given file. */
int nx_fs_read(fs_fd_t fd) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  /* Compute the next file->rpos and return what's there. */
  return *(file->rpos);
}

/* Write one byte to the given file. */
fs_err_t nx_fs_write(fs_fd_t fd, int byte) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (file->wpos == FS_BUF_SIZE-1) {
    fs_err_t err = nx_fs_flush(fd);
    if (err != FS_ERR_NO_ERROR)
      return err;
  }

  file->wbuf[file->wpos++] = byte;
  return FS_ERR_NO_ERROR;
}

/* Flush the write buffer of the given file. */
fs_err_t nx_fs_flush(fs_fd_t fd) {
  volatile fs_file_t *file = nx_fs_get_file(fd);
  
  if (file->wpos > 0) {
    /* All the logic for writing the page at the correct
     * place in the flash should go here.
     */
    
    file->wpos = 0;
  }

  return FS_ERR_NO_ERROR;
}

/* Close a file. */
fs_err_t nx_fs_close(fs_fd_t fd) {
  volatile fs_file_t *file;
  fs_err_t err;
  
  file = nx_fs_get_file(fd);
  err = nx_fs_flush(fd);

  if (err != FS_ERR_NO_ERROR)
    return err;

  file->entry.opened = FALSE;
  return FS_ERR_NO_ERROR;
}

