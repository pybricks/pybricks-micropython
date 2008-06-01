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

#include "base/display.h"

extern fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Returns a file info structure given its file descriptor,
 * or NULL if the fd is invalid.
 */
fs_file_t *nx__fs_get_file(fs_fd_t fd) {
  NX_ASSERT(fd < FS_MAX_OPENED_FILES);

  if (!fdset[fd].used) {
    return NULL;
  }

  return &(fdset[fd]);
}

/* Determines if the given page contains a file origin marker.
 */
inline bool nx__fs_page_has_magic(U32 page) {
  return ((FLASH_BASE_PTR[page*EFC_PAGE_WORDS] & FS_FILE_ORIGIN_MASK) >> 24) == FS_FILE_ORIGIN_MARKER;
}

/* Find a file's origin on the file system by its name.
 */
fs_err_t nx__fs_find_file_origin(char *name, U32 *origin) {
  U32 i;

  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx__fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);
      union U32tochar nameconv;

      memcpy(nameconv.integers,
             (void *)(metadata + FS_FILENAME_OFFSET),
             FS_FILENAME_LENGTH);

      if (streq(nameconv.chars, name)) {
        *origin = i;
        return FS_ERR_NO_ERROR;
      }

      /* Otherwise jump over the file and continue searching. */
      else {
        i += nx__fs_get_file_page_count(
          nx__fs_get_file_size_from_metadata(metadata)) - 1;
      }
    }
  }

  return FS_ERR_FILE_NOT_FOUND;
}

/* Finds the last file origin on the flash.
 */
fs_err_t nx__fs_find_last_origin(U32 *origin) {
  U32 candidate = 0, i;

  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx__fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);

      candidate = i;
      i += nx__fs_get_file_page_count(
        nx__fs_get_file_size_from_metadata(metadata)) - 1;
    }
  }

  if (candidate) {
    *origin = candidate;
    return FS_ERR_NO_ERROR;
  }

  return FS_ERR_FILE_NOT_FOUND;
}

fs_err_t nx__fs_find_next_origin(U32 start, U32 *origin) {
  U32 i;

  for (i=start; i<FS_PAGE_END; i++) {
    if (nx__fs_page_has_magic(i)) {
      *origin = i;
      return FS_ERR_NO_ERROR;
    }
  }

  return FS_ERR_FILE_NOT_FOUND;
}

/* Returns the number of pages used by a file, given its size.
 */
U32 nx__fs_get_file_page_count(size_t size) {
  U32 pages;

  /* Compute page occupation. */
  size += FS_FILE_METADATA_BYTES;
  pages = size / EFC_PAGE_BYTES;
  if (size % EFC_PAGE_BYTES) {
    pages++;
  }

  return pages;
}

size_t nx__fs_get_file_size_from_metadata(volatile U32 *metadata) {
  return *metadata & FS_FILE_SIZE_MASK;
}

fs_perm_t nx__fs_get_file_perms_from_metadata(volatile U32 *metadata) {
  U8 perms = (*metadata & FS_FILE_PERMS_MASK) >> 20;

  if (perms & FS_FILE_PERM_MASK_READWRITE) {
    return FS_PERM_READWRITE;
  } else if (perms & FS_FILE_PERM_MASK_EXECUTABLE) {
    return FS_PERM_EXECUTABLE;
  }

  /* Defaults to read-only. */
  return FS_PERM_READONLY;
}

/* Serialize a file's metadata using the provided values and returns
 * the resulting U32s, ready to be stored on flash.
 */
void nx__fs_create_metadata(fs_perm_t perms, char *name, size_t size,
                            U32 *metadata) {
  union U32tochar nameconv;

  memset(metadata, 0, FS_FILE_METADATA_BYTES);

  memset(nameconv.chars, 0, 32);
  memcpy(nameconv.chars, name, MIN(strlen(name), 31));

  /* Magic marker. */
  metadata[0] = (FS_FILE_ORIGIN_MARKER << 24);

  /* File permissions. */
  switch (perms) {
    case FS_PERM_READWRITE:
      metadata[0] += (FS_FILE_PERM_MASK_READWRITE << 20);
      break;
    case FS_PERM_EXECUTABLE:
      metadata[0] += (FS_FILE_PERM_MASK_EXECUTABLE << 20);
      break;
    default:
      break;
  }

  /* File size. */
  metadata[0] += (size & FS_FILE_SIZE_MASK);

  /* Insert here in metadata[1] what would be relevant (future). */

  /* File name. */
  memcpy(metadata + FS_FILENAME_OFFSET, nameconv.integers, FS_FILENAME_LENGTH);
}

fs_err_t nx__fs_relocate_to_page(fs_file_t *file, U32 origin) {
  U32 page_data[EFC_PAGE_WORDS], null_data[EFC_PAGE_WORDS] = {0};
  U32 diff, i;
  size_t size;

  diff = origin - file->origin;

  /* Move the file's data. */
  size = nx__fs_get_file_page_count(file->size);

  for (i=file->origin; i<size; i++) {
    nx__efc_read_page(i, page_data);
    /* TODO: figure out if we want to erase the source page now or later. */
    if (!nx__efc_write_page(page_data, i + diff)
      || !nx__efc_write_page(null_data, i)) {
      return FS_ERR_FLASH_ERROR;
    }
  }

  file->origin = origin;
  file->rbuf.page += diff;
  file->wbuf.page += diff;

  return FS_ERR_NO_ERROR;
}

fs_err_t nx__fs_relocate(fs_file_t *file) {
  U32 origin, start;
  size_t size;

  size = nx__fs_get_file_page_count(file->size);

  /* First, look at the end of the flash for free space. */
  if (nx__fs_find_last_origin(&origin) == FS_ERR_NO_ERROR) {
    origin += nx__fs_get_file_page_count(
                nx__fs_get_file_size_from_metadata(
                  &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS])));
	
    if (size < FS_PAGE_END - origin) {
      return nx__fs_relocate_to_page(file, origin);
    }
  }

  start = FS_PAGE_START;
  while (nx__fs_find_next_origin(start, &origin) != FS_ERR_FILE_NOT_FOUND) {
    if (size < origin - start || (origin == file->origin && file->origin - start > 0)) {
      return nx__fs_relocate_to_page(file, origin);
	  }
	
	  start = origin + nx__fs_get_file_page_count(
                nx__fs_get_file_size_from_metadata(
                  &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS])));
  }

  return FS_ERR_NO_SPACE_LEFT_ON_DEVICE;
}


