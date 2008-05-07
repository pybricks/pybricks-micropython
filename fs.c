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
#include "base/_fs.h"
#include "base/drivers/_efc.h"

extern volatile fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Initialize the file system, most importantly check for file system
 * integrity?. */
fs_err_t nx_fs_init(void) {
  return FS_ERR_NO_ERROR;
}

/* Initializes the @a fd fdset slot with the file's metadata.
 */
static fs_err_t nx_fs_init_fd(U32 origin, fs_fd_t fd) {
  volatile U32 *metadata = &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS]);
  volatile fs_file_t *file;
  
  file = nx_fs_get_file(fd);
  NX_ASSERT(file != NULL);
  
  memcpy((void *)file->name, (void *)(metadata+1), FS_FILENAME_LENGTH);
  file->origin = origin;
  file->size = metadata[FS_FILENAME_LENGTH+2];
  file->rpos = metadata + FS_FILE_METADATA_SIZE;

  return FS_ERR_NO_ERROR;
}

/* Open an existing file by its name. */
static fs_err_t nx_fs_open_by_name(char *name, fs_fd_t fd) {
  U32 origin;
  fs_err_t err;
  
  err = nx__fs_find_file_origin(name, &origin);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }
  
  return nx_fs_init_fd(origin, fd);
}

/* Create a new file using the given name. */
static fs_err_t nx_fs_create_by_name(char *name, fs_fd_t fd) {
  U32 origin = 0;
  
  /* Find an origin page. */
  /* Bootstrap the metadata to the flash page. */

  // TO BE REMOVED
  NX_ASSERT(strlen(name));
  
  return nx_fs_init_fd(origin, fd);
}

/* Open or create a file by its name. The associated file descriptor
 * is returned via the fd pointer argument.
 */
fs_err_t nx_fs_open(char *name, fs_file_mode_t mode, fs_fd_t *fd) {
  volatile fs_file_t *file;
  U8 slot = 0;
  fs_err_t err;

  NX_ASSERT(strlen(name) > 0);
  NX_ASSERT(strlen(name) < FS_FILENAME_LENGTH);

  /* First, make sure we have an avaliable slot for this file. */
  while (slot < FS_MAX_OPENED_FILES && fdset[slot].used) {
   slot++;
  }

  if (slot == FS_MAX_OPENED_FILES) {
    return FS_ERR_TOO_MANY_OPENED_FILES;
  }

  /* Reserve it. */
  file = &(fdset[slot]);
  file->used = TRUE;
  
  switch (mode) {
  case FS_FILE_MODE_OPEN:
    err = nx_fs_open_by_name(name, slot);
    break;
  case FS_FILE_MODE_CREATE:
    err = nx_fs_create_by_name(name, slot);
    break;
  default:
    err = FS_ERR_UNSUPPORTED_MODE;
    break;
  }
  
  if (err == FS_ERR_NO_ERROR) {
    *fd = slot;
  } else {
    /* Otherwise release the slot that was reserved. */
    file->used = FALSE;
  }
  
  return err;
}

/* Get the file size */
size_t nx_fs_get_filesize(fs_fd_t fd) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (!file) {
    return -1;
  }
  
  return file->size;
}

/* Read one byte from the given file. */
fs_err_t nx_fs_read(fs_fd_t fd, U32 *byte) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  /* Detect end of file. */
  if (file->rpos - &(FLASH_BASE_PTR[file->origin])
      + FS_FILE_METADATA_SIZE <= 0) {
    return FS_ERR_END_OF_FILE;
  }

  *byte = *file->rpos;
  file->rpos++;

  return FS_ERR_NO_ERROR;
}

/* Write one byte to the given file. */
fs_err_t nx_fs_write(fs_fd_t fd, U32 byte) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (!file) {
    return FS_ERR_INVALID_FD;
  }

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
  
  if (!file) {
    return FS_ERR_INVALID_FD;
  }
  
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
  if (!file) {
    return FS_ERR_INVALID_FD;
  }
  
  file->used = FALSE;
  err = nx_fs_flush(fd);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }

  return FS_ERR_NO_ERROR;
}

fs_perm_t nx_fs_get_perms(fs_fd_t fd) {
  volatile fs_file_t *file;
  
  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  return file->perms;
}

fs_err_t nx_fs_set_perms(fs_fd_t fd, fs_perm_t perms) {
  volatile fs_file_t *file;
  
  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }
  
  file->perms = perms;
  
  /* TODO: sync metadata. */
  
  return FS_ERR_NO_ERROR;
}

/** TODO: test! */
static U32 nx_fs_get_file_page_count(volatile fs_file_t *file) {
  U32 pages;
  
  NX_ASSERT(file != NULL);
  
  pages = file->size / EFC_PAGE_WORDS;
  if (file->size % EFC_PAGE_WORDS) {
    pages++;
  }
  
  return pages;
}

fs_err_t nx_fs_unlink(fs_fd_t fd) {
  volatile fs_file_t *file;
  U32 page, end;
  
  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }
  
  /* Remove file marker and potential in-file marker-alike. */
  end = file->origin + nx_fs_get_file_page_count(file);
  for (page = file->origin; page <= end; page++) {
    if (nx_fs_page_has_magic(page)) {
      /* Erase marker. */
    }
  }
  
  file->used = FALSE;
  return FS_ERR_NO_ERROR;
}

