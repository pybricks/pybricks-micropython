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
#include "base/assert.h"
#include "base/util.h"
#include "base/fs.h"
#include "base/_fs.h"
#include "base/drivers/_efc.h"

extern volatile fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Returns a file info structure given its file descriptor,
 * or NULL if the fd is invalid.
 */
volatile fs_file_t *nx_fs_get_file(fs_fd_t fd) {
  NX_ASSERT(fd > 0);

  if (fd >= FS_MAX_OPENED_FILES || !fdset[fd].used) {
    return NULL;
  }

  return &(fdset[fd]);
}

/* Determines if the given page contains a file origin marker.
 */
inline bool nx_fs_page_has_magic(U32 page) {
  return FLASH_BASE_PTR[page*EFC_PAGE_WORDS] == FS_FILE_ORIGIN_MARKER;
}

/* Find a file's origin on the file system by its name.
 */
fs_err_t nx__fs_find_file_origin(char *name, U32 *origin) {
  U16 i;
  
  for (i=0; i<EFC_PAGES; i++) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);

      if (strncmp(name, (char *)(metadata+1), FS_FILENAME_LENGTH) == 0) {
        *origin = i;
        return FS_ERR_NO_ERROR;
      }
      /* Otherwise jump over the file and continue searching. */
      else {
        size_t size = metadata[FS_FILENAME_LENGTH+2];
        i += size % EFC_PAGE_WORDS;
      }
    }
  }

  return FS_ERR_FILE_NOT_FOUND;
}
