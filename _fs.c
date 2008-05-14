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

extern volatile fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Returns a file info structure given its file descriptor,
 * or NULL if the fd is invalid.
 */
volatile fs_file_t *nx_fs_get_file(fs_fd_t fd) {
  NX_ASSERT(fd < FS_MAX_OPENED_FILES);
  
  if (!fdset[fd].used) {
    return NULL;
  }

  return &(fdset[fd]);
}

/* Determines if the given page contains a file origin marker.
 */
inline bool nx_fs_page_has_magic(U32 page) {
  return ((FLASH_BASE_PTR[page*EFC_PAGE_WORDS] & FS_FILE_ORIGIN_MASK) >> 24) == FS_FILE_ORIGIN_MARKER;
}

/*
 */
fs_err_t seek_page_from_position(U8 pos, fs_fd_t fd) {
	U16 page;
	volatile fs_file_t *file;
  
	file = nx_fs_get_file(fd);
	if(!file) {
		return FS_ERR_INVALID_FD;
	}
	
	if(pos > file->size) {
		return FS_ERR_INCORRECT_POS;
	}
	
	page = (pos + FS_FILE_METADATA_BYTES) / EFC_PAGE_BYTES;
	
	if(file->rbuf.page != page) {
		nx__efc_read_page(page, file->rbuf.data.raw);
		file->rbuf.page = page;
	}
	
	file->rbuf.pos = pos;
	
	return FS_ERR_NO_ERROR;
}

/* Find a file's origin on the file system by its name.
 */
fs_err_t nx__fs_find_file_origin(char *name, U32 *origin) {
  U16 i;
  
  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);
      union U32tochar nameconv;
      
      memcpy(nameconv.integers,
             (void *)(metadata + FS_FILENAME_OFFSET),
             FS_FILENAME_SIZE);
      
      if (strcmp(nameconv.chars, name) == 0) {
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

fs_err_t nx__fs_find_last_origin(U32 *origin) {
  U32 candidate = 0;
  U16 i;
  
  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
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

U32 nx__fs_get_file_page_count(size_t size) {
  U32 pages;
  
  /* Compute page occupation. */
  pages = (FS_FILE_METADATA_BYTES + size) / EFC_PAGE_BYTES;
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

void nx__fs_compute_occupation(U16 *files, U32 *used, U32 *free_pages,
                               U32 *wasted) {
  if (files) {
  }
  
  if (used) {
  }
  
  if (free_pages) {
  }
  
  if (wasted) {
  }
}

void nx__fs_create_metadata(fs_perm_t perms, char *name, size_t size,
                            U32 *metadata) {
  union U32tochar nameconv;

  memset(metadata, 0, FS_FILE_METADATA_BYTES);
  
  memset(nameconv.chars, 0, 32);
  memcpy(nameconv.chars, name, MIN(strlen(name), 31));

  metadata[0] = (FS_FILE_ORIGIN_MARKER << 24);
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
  
  metadata[0] += (size & FS_FILE_SIZE_MASK);
  
  /* Insert here in metadata[1] what would be relevant (future). */
  
  memcpy(metadata + FS_FILENAME_OFFSET, nameconv.integers, 8);
}
