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

/* File descriptors set, limited to FS_MAX_OPENED_FILES. Each file
 * descriptor (U32) is linked to its fs_file_t structured through
 * this array of opened files. */
static fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Initialize the file system, most importantly check for file system
 * integrity. */
fs_err_t nx_fs_init(void) {
  return FS_ERR_NO_ERROR;
}

/* Open a file by its name and return the file descriptor of the opened
 * file, or -1 if the file could not be opened (too many opened files,
 * file not found, etc).
 */
fs_err_t nx_fs_open(char *name, U32 *fd) {
  U32 i, file = FS_MAX_OPENED_FILES;
  U32 name_len;

  name_len = strlen(name);

  NX_ASSERT(name_len > 0);
  NX_ASSERT(name_len <= FS_FILENAME_LENGTH);

  for (i=0 ; i<FS_MAX_OPENED_FILES ; i++) {
    if (fdset[i]._used == TRUE) {
      /* If the file is already opened, return its file descriptor. */
      if (strcmp(name, fdset[i].name) == 0)
        return i;
    } else if (file == FS_MAX_OPENED_FILES) {
      /* Otherwise find the first available slot in the fdset. */
      file = i;
    }
  }

  /* If no available slot was found, return -1 to note the too many
   * open files error. */
  if (file >= FS_MAX_OPENED_FILES)
    return FS_ERR_TOO_MANY_OPENED_FILES;

  /* Check that the file exists. */
  if (0)
    return FS_ERR_FILE_NOT_FOUND;
  
  fdset[file]._used = TRUE;

  memcpy(fdset[file].name, name, name_len);
  fdset[file].name[name_len] = 0;

  /* Read file attributes (size, permissions). */
  
  *fd = file;
  return FS_ERR_NO_ERROR;
}

void nx_fs_close(U32 fd) {
  NX_ASSERT(fd > 0);
  NX_ASSERT(fd < FS_MAX_OPENED_FILES);

  /* Synchronize file buffers ? */

  fdset[fd]._used = FALSE;
}

