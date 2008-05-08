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
  
  file->origin = origin;
  file->size = nx__fs_get_file_size_from_metadata(metadata);
  file->perms = nx__fs_get_file_perms_from_metadata(metadata);
  
  memset(file->rbuf, 0, sizeof(fs_buffer_t));
  memset(file->wbuf, 0, sizeof(fs_buffer_t));

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
  U32 metadata[EFC_PAGE_WORDS] = {0};
  U32 origin;

  /* Find an origin page. */
  if (nx__fs_find_last_origin(&origin) == FS_ERR_NO_ERROR) {
    origin += nx__fs_get_file_page_count(
                nx__fs_get_file_size_from_metadata(
                  &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS])));
  } else {
     origin = FS_PAGE_START;
  }

  if (origin >= EFC_PAGES) {
    /* TODO: search for an available page on the flash. */
    
    return FS_ERR_NO_SPACE_LEFT_ON_DEVICE;
  }
  
  /* Bootstrap the metadata to the flash page. */
  nx__fs_create_metadata(FS_PERM_READWRITE, name, 0, metadata);

  /* Write metadata to flash. */
  if (!nx__efc_write_page(metadata, origin)) {
    return FS_ERR_FLASH_ERROR;
  }
  
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
    case FS_FILE_MODE_CREATE:
      err = nx_fs_create_by_name(name, slot);
      
      nx__efc_read_page(file->origin, file->wbuf.data.raw);
      file->wbuf.pos = FS_FILE_METADATA_SIZE;
      file->wbuf.page = file->origin;
            
      memcpy((void *)file->rbuf, (void *)file->wbuf, sizeof(fs_buffer_t));
      break;
    case FS_FILE_MODE_OPEN:
      err = nx_fs_open_by_name(name, slot);

      nx__efc_read_page(file->origin, file->wbuf.data.raw);      
      file->wbuf.pos = FS_FILE_METADATA_SIZE;
      file->wbuf.page = file->origin;

      memcpy((void *)file->rbuf, (void *)file->wbuf, sizeof(fs_buffer_t));
      break;
    case FS_FILE_MODE_APPEND:
      err = nx_fs_open_by_name(name, slot);
      
      /* Put writing position at the end of the file. */
      file->wbuf.page = file->origin
        + nx__fs_get_file_page_count(file->size) - 1;
      nx__efc_read_page(file->wbuf.page, file->wbuf.data.raw);
      file->wbuf.pos = (FS_FILE_METADATA_BYTES + file->size) % EFC_PAGE_BYTES;
      
      nx__efc_read_page(file->origin, file->rbuf.data.raw);      
      file->rbuf.pos = FS_FILE_METADATA_SIZE;
      file->rbuf.page = file->origin;
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

/* Get the file size, in bytes. */
size_t nx_fs_get_filesize(fs_fd_t fd) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (!file) {
    return -1;
  }
  
  return file->size;
}

/* Read one byte from the given file. */
fs_err_t nx_fs_read(fs_fd_t fd, U8 *byte) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  /* Detect end of file. */
  if (file->rbuf.page * EFC_PAGE_BYTES
      + file->rbuf.pos > FS_FILE_METADATA_BYTES + file->size) {
    return FS_ERR_END_OF_FILE;
  }

  /* If needed, update buffer. */
  if (file->rbuf.pos == FS_BUF_SIZE) {
    file->rbuf.page++;
    file->rbuf.pos = 0;
    nx__efc_read_page(file->rbuf.page, file->rbuf.data.raw);
  }

  *byte = file->rbuf.data.bytes[file->rbuf.pos++];
  return FS_ERR_NO_ERROR;
}

/* Write one byte to the given file. */
fs_err_t nx_fs_write(fs_fd_t fd, U8 byte) {
  volatile fs_file_t *file = nx_fs_get_file(fd);

  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  if (file->wbuf.pos == FS_BUF_SIZE) {
    fs_err_t err = nx_fs_flush(fd);
    if (err != FS_ERR_NO_ERROR)
      return err;
    
    file->wbuf.page++;
    file->wbuf.pos = 0;
  }

  file->wbuf.data.bytes[file->wpos++] = byte;
  
  // BLEH
  file->size++;

  return FS_ERR_NO_ERROR;
}

/* Flush the write buffer of the given file. */
fs_err_t nx_fs_flush(fs_fd_t fd) {
  volatile fs_file_t *file = nx_fs_get_file(fd);
  U32 firstpage[EFC_PAGE_WORDS];
  U16 page;
  
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  // BLEH
  page = file->origin
    + (FS_FILE_METADATA_BYTES + file->size) / EFC_PAGE_BYTES - 1;
  if (nx__fs_page_has_magic()) {}

  /* Update file metadata on flash. */
  firstpage = nx__efc_read_page(file->origin, firstpage);
  nx__fs_create_metadata(file->perms, file->name, file->size, firstpage);
  nx__efc_write_page(file->origin, firstpage);

  if (file->wbuf.pos == 0) {
    return FS_ERR_NO_ERROR;
  }



  /* Write page data. */
  

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

fs_err_t nx_fs_unlink(fs_fd_t fd) {
  volatile fs_file_t *file;
  U32 page, end;
  
  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }
  
  /* Remove file marker and potential in-file marker-alike. */
  end = file->origin + nx__fs_get_file_page_count(file->size);
  for (page = file->origin; page < end; page++) {
    if (nx_fs_page_has_magic(page)) {
      /* Erase marker. */
    }
  }
  
  file->used = FALSE;
  return FS_ERR_NO_ERROR;
}

