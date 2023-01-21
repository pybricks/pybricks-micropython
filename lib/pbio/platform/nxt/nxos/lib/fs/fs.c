/* Copyright (c) 2008 the NxOS developers
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
#include "base/display.h"
#include "base/drivers/_efc.h"

#include "base/lib/fs/fs.h"

/* Magic marker. */
#define FS_FILE_ORIGIN_MARKER 0x42

/* File metadata size, in U32s. */
#define FS_FILE_METADATA_SIZE 10

/** Filename offset (in U32s) in the metadata. */
#define FS_FILENAME_OFFSET 2

/** File metadata size, in bytes. */
#define FS_FILE_METADATA_BYTES (FS_FILE_METADATA_SIZE * sizeof(U32))

/** Mask to use on the first metadata U32 to get the file origin marker value. */
#define FS_FILE_ORIGIN_MASK 0xFF000000

/** Mask to use on the first metadata U32 to get the file permissions. */
#define FS_FILE_PERMS_MASK 0x00F00000

/** Mask to use on the first metadata U32 to get the file size. */
#define FS_FILE_SIZE_MASK 0x000FFFFF

#define FS_FILE_PERM_MASK_READWRITE (1 << 0)
#define FS_FILE_PERM_MASK_EXECUTABLE (1 << 1)

/** U32 <-> char conversion union for filenames. */
union U32tochar {
  char chars[FS_FILENAME_LENGTH];
  U32 integers[FS_FILENAME_SIZE];
};

/* FD-set. */
static fs_file_t fdset[FS_MAX_OPENED_FILES];

/* Returns a file info structure given its file descriptor,
 * or NULL if the fd is invalid.
 */
static fs_file_t *nx_fs_get_file(fs_fd_t fd) {
  NX_ASSERT(fd < FS_MAX_OPENED_FILES);

  if (!fdset[fd].used) {
    return NULL;
  }

  return &(fdset[fd]);
}

/* Determines if the given page contains a file origin marker.
 */
inline static bool nx_fs_page_has_magic(U32 page) {
  return ((FLASH_BASE_PTR[page*EFC_PAGE_WORDS] & FS_FILE_ORIGIN_MASK) >> 24)
    == FS_FILE_ORIGIN_MARKER;
}

/* Returns the number of pages used by a file, given its size.
 */
static U32 nx_fs_get_file_page_count(size_t size) {
  U32 pages;

  /* Compute page occupation. */
  size += FS_FILE_METADATA_BYTES;
  pages = size / EFC_PAGE_BYTES;
  if (size % EFC_PAGE_BYTES) {
    pages++;
  }

  return pages;
}

inline static size_t nx_fs_get_file_size_from_metadata(volatile U32 *metadata) {
  return *metadata & FS_FILE_SIZE_MASK;
}

static fs_perm_t nx_fs_get_file_perms_from_metadata(volatile U32 *metadata) {
  U8 perms = (*metadata & FS_FILE_PERMS_MASK) >> 20;

  if (perms & FS_FILE_PERM_MASK_READWRITE) {
    return FS_PERM_READWRITE;
  } else if (perms & FS_FILE_PERM_MASK_EXECUTABLE) {
    return FS_PERM_EXECUTABLE;
  }

  /* Defaults to read-only. */
  return FS_PERM_READONLY;
}

/* Find a file's origin on the file system by its name.
 */
static fs_err_t nx_fs_find_file_origin(char *name, U32 *origin) {
  U32 i;

  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
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
        i += nx_fs_get_file_page_count(
          nx_fs_get_file_size_from_metadata(metadata)) - 1;
      }
    }
  }

  return FS_ERR_FILE_NOT_FOUND;
}

/* Finds the last file origin on the flash.
 */
static fs_err_t nx_fs_find_last_origin(U32 *origin) {
  U32 candidate = 0, i;

  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);

      candidate = i;
      i += nx_fs_get_file_page_count(
        nx_fs_get_file_size_from_metadata(metadata)) - 1;
    }
  }

  if (candidate) {
    *origin = candidate;
    return FS_ERR_NO_ERROR;
  }

  return FS_ERR_FILE_NOT_FOUND;
}

static fs_err_t nx_fs_find_next_origin(U32 start, U32 *origin) {
  U32 i;

  for (i=start; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
      *origin = i;
      return FS_ERR_NO_ERROR;
    }
  }

  return FS_ERR_FILE_NOT_FOUND;
}

/* Serialize a file's metadata using the provided values and returns
 * the resulting U32s, ready to be stored on flash.
 */
static void nx_fs_create_metadata(fs_perm_t perms, char *name, size_t size,
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

static fs_err_t nx_fs_move_region_backwards(U32 source, U32 dest, U32 len) {
  U32 data[EFC_PAGE_WORDS];

  while (len--) {
    nx__efc_read_page(source, data);

    if (!nx__efc_write_page(data, dest)) {
      return FS_ERR_FLASH_ERROR;
    }

    if (!nx__efc_erase_page(source, 0)) {
      return FS_ERR_FLASH_ERROR;
    }

    source++;
    dest++;
  }

  return FS_ERR_NO_ERROR;
}

static fs_err_t nx_fs_move_region_forwards(U32 source, U32 dest, U32 len) {
  U32 data[EFC_PAGE_WORDS];

  while (len--) {
    nx__efc_read_page(source + len, data);

    if (!nx__efc_write_page(data, dest + len)) {
      return FS_ERR_FLASH_ERROR;
    }

    if (!nx__efc_erase_page(source + len, 0)) {
      return FS_ERR_FLASH_ERROR;
    }
  }

  return FS_ERR_NO_ERROR;
}

/* Move a @a len long flash region starting at page @a source to @a dest.
 * Since pages are moved one after another, regions may overlap if the
 * destination is lower in the flash than the source, but not the other
 * way around. Note that this is asserted anyway to avoid data loss.
 * It is the responsibility of the caller to clean up the remaining
 * source region of any data he doesn't want to leave there (file origin
 * markers for example).
 *
 * @param source The source page number.
 * @param dest The destination page number.
 * @param len The region length.
 */
static fs_err_t nx_fs_move_region(U32 source, U32 dest, U32 len) {
  NX_ASSERT(source < EFC_PAGES);
  NX_ASSERT(dest < EFC_PAGES);
  NX_ASSERT(len < EFC_PAGES);

  if (source == dest) {
    return FS_ERR_NO_ERROR;
  } else if (dest < source) {
    return nx_fs_move_region_backwards(source, dest, len);
  } else {
    return nx_fs_move_region_forwards(source, dest, len);
  }
}

/* Relocate the given file to @a origin.
 */
static fs_err_t nx_fs_relocate_to_page(fs_file_t *file, U32 origin) {
  fs_err_t err;
  size_t n_pages;
  U32 diff;

  diff = origin - file->origin;

  /* Move the file's data. */
  n_pages = nx_fs_get_file_page_count(file->size);
  err = nx_fs_move_region(file->origin, origin, n_pages);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }

  /* Set the file pages pointers to their respective new values. */
  file->origin = origin;
  file->rbuf.page += diff;
  file->wbuf.page += diff;

  return FS_ERR_NO_ERROR;
}

static fs_err_t nx_fs_relocate(fs_file_t *file) {
  U32 origin, start;
  size_t size;

  size = nx_fs_get_file_page_count(file->size);

  /* First, look at the end of the flash for free space. */
  if (nx_fs_find_last_origin(&origin) == FS_ERR_NO_ERROR) {
    origin += nx_fs_get_file_page_count(
        nx_fs_get_file_size_from_metadata(
          &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS])));

    if (size < FS_PAGE_END - origin) {
      return nx_fs_relocate_to_page(file, origin);
    }
  }

  start = FS_PAGE_START;
  while (nx_fs_find_next_origin(start, &origin) != FS_ERR_FILE_NOT_FOUND) {
    if (size < origin - start || (origin == file->origin && file->origin - start > 0)) {
      return nx_fs_relocate_to_page(file, origin);
    }

    start = origin + nx_fs_get_file_page_count(
        nx_fs_get_file_size_from_metadata(
          &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS])));
  }

  return FS_ERR_NO_SPACE_LEFT_ON_DEVICE;
}

/* Initialize the file system, most importantly check for file system
 * integrity?. */
fs_err_t nx_fs_init(void) {
  return FS_ERR_NO_ERROR;
}

/* Initializes the @a fd fdset slot with the file's metadata.
 */
static fs_err_t nx_fs_init_fd(U32 origin, fs_fd_t fd) {
  volatile U32 *metadata = &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS]);
  union U32tochar nameconv;
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  NX_ASSERT(file != NULL);

  memcpy(nameconv.integers,
         (void *)(metadata + FS_FILENAME_OFFSET),
         FS_FILENAME_LENGTH);

  file->origin = origin;
  memcpy(file->name, nameconv.chars, MIN(strlen(nameconv.chars), 31));
  file->size = nx_fs_get_file_size_from_metadata(metadata);
  file->perms = nx_fs_get_file_perms_from_metadata(metadata);

  file->rbuf.page = file->rbuf.pos = 0;
  file->wbuf.page = file->wbuf.pos = 0;
  memset(file->rbuf.data.bytes, 0, EFC_PAGE_BYTES);
  memset(file->wbuf.data.bytes, 0, EFC_PAGE_BYTES);

  return FS_ERR_NO_ERROR;
}

/* Open an existing file by its name. */
static fs_err_t nx_fs_open_by_name(char *name, fs_fd_t fd) {
  U32 origin;
  fs_err_t err;

  err = nx_fs_find_file_origin(name, &origin);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }

  return nx_fs_init_fd(origin, fd);
}

/* Create a new file using the given name. */
static fs_err_t nx_fs_create_by_name(char *name, fs_fd_t fd) {
  U32 metadata[EFC_PAGE_WORDS] = {0};
  U32 origin;
  fs_err_t err;

  /* Check that a file by that name does not already exists. */
  err = nx_fs_find_file_origin(name, &origin);
  if (err != FS_ERR_FILE_NOT_FOUND) {
    return FS_ERR_FILE_ALREADY_EXISTS;
  }

  /* Find an origin page. */
  if (nx_fs_find_last_origin(&origin) == FS_ERR_NO_ERROR) {
    origin += nx_fs_get_file_page_count(
        nx_fs_get_file_size_from_metadata(
          &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS])));
  } else {
    origin = FS_PAGE_START;
  }

  if (origin >= EFC_PAGES) {
    /* TODO: search for an available page on the flash. */

    return FS_ERR_NO_SPACE_LEFT_ON_DEVICE;
  }

  /* Bootstrap the metadata to the flash page. */
  nx_fs_create_metadata(FS_PERM_READWRITE, name, 0, metadata);

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
  fs_file_t *file;
  fs_err_t err;
  U8 slot = 0;

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
      if (err != FS_ERR_NO_ERROR) {
        break;
      }

      nx__efc_read_page(file->origin, file->wbuf.data.raw);
      file->wbuf.pos = FS_FILE_METADATA_BYTES;
      file->wbuf.page = file->origin;

      file->rbuf = file->wbuf;
      break;
    case FS_FILE_MODE_OPEN:
      err = nx_fs_open_by_name(name, slot);
      if (err != FS_ERR_NO_ERROR) {
        break;
      }

      nx__efc_read_page(file->origin, file->wbuf.data.raw);
      file->wbuf.pos = FS_FILE_METADATA_BYTES;
      file->wbuf.page = file->origin;

      file->rbuf = file->wbuf;
      break;
    case FS_FILE_MODE_APPEND:
      err = nx_fs_open_by_name(name, slot);
      if (err != FS_ERR_NO_ERROR) {
        break;
      }

      /* Put writing position at the end of the file. */
      file->wbuf.page = file->origin
        + nx_fs_get_file_page_count(file->size) - 1;
      nx__efc_read_page(file->wbuf.page, file->wbuf.data.raw);
      file->wbuf.pos = (FS_FILE_METADATA_BYTES + file->size) % EFC_PAGE_BYTES;

      file->rbuf.page = file->origin;
      nx__efc_read_page(file->rbuf.page, file->rbuf.data.raw);
      file->rbuf.pos = FS_FILE_METADATA_BYTES;

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
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  if (!file) {
    return -1;
  }

  return file->size;
}

/* Read one byte from the given file. */
fs_err_t nx_fs_read(fs_fd_t fd, U8 *byte) {
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  /* Detect end of file. */
  if ((file->rbuf.page - file->origin) * EFC_PAGE_BYTES
      + file->rbuf.pos > FS_FILE_METADATA_BYTES + file->size) {
    return FS_ERR_END_OF_FILE;
  }

  /* If needed, update buffer. */
  if (file->rbuf.pos == EFC_PAGE_BYTES) {
    file->rbuf.page++;
    file->rbuf.pos = 0;
    memset(file->rbuf.data.bytes, 0, EFC_PAGE_BYTES);

    nx__efc_read_page(file->rbuf.page, file->rbuf.data.raw);
  }

  *byte = file->rbuf.data.bytes[file->rbuf.pos++];
  return FS_ERR_NO_ERROR;
}

/* Write one byte to the given file. */
fs_err_t nx_fs_write(fs_fd_t fd, U8 byte) {
  fs_file_t *file;
  fs_err_t err;
  U32 pages;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  pages = nx_fs_get_file_page_count(file->size) - 1;

  /* Check that the page we will be writing to is available,
   * aka its either "inside" the file itself, either after but
   * free.
   */
  if (file->wbuf.pos == 0 &&
      file->wbuf.page > file->origin + pages &&
      nx_fs_page_has_magic(file->wbuf.page)) {
    /* If the page we want to use is not available relocate the file. */
    err = nx_fs_relocate(file);
    if (err != FS_ERR_NO_ERROR) {
      return err;
    }
  }

  /* If needed, flush the write buffer to the flash and reinit it. */
  if (file->wbuf.pos == EFC_PAGE_BYTES) {
    err = nx_fs_flush(fd);
    if (err != FS_ERR_NO_ERROR)
      return err;

    file->wbuf.page++;
    file->wbuf.pos = 0;
    memset(file->wbuf.data.bytes, 0, EFC_PAGE_BYTES);

    /* We now have one more page for this file (value used below). */
    pages++;
  }

  file->wbuf.data.bytes[file->wbuf.pos++] = byte;

  /* Increment the size of the file if necessary */
  if (file->wbuf.page == file->origin + pages) {
    if (file->wbuf.pos > file->size + FS_FILE_METADATA_BYTES
        - (pages * EFC_PAGE_BYTES)) {
      file->size++;
    }
  }

  return FS_ERR_NO_ERROR;
}

/* Flush the write buffer of the given file. */
fs_err_t nx_fs_flush(fs_fd_t fd) {
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  /* Write the page. */
  if (!nx__efc_write_page(file->wbuf.data.raw, file->wbuf.page)) {
    return FS_ERR_FLASH_ERROR;
  }

  if (file->wbuf.pos == 0) {
    return FS_ERR_NO_ERROR;
  }

  /* Write page data. */

  return FS_ERR_NO_ERROR;
}

/* Close a file. */
fs_err_t nx_fs_close(fs_fd_t fd) {
  U32 firstpage[EFC_PAGE_WORDS];
  fs_file_t *file;
  fs_err_t err;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  err = nx_fs_flush(fd);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }

  /* Update the file's metadata. */
  nx__efc_read_page(file->origin, firstpage);
  nx_fs_create_metadata(file->perms, file->name, file->size, firstpage);
  if (!nx__efc_write_page(firstpage, file->origin)) {
    return FS_ERR_FLASH_ERROR;
  }

  file->used = FALSE;
  return FS_ERR_NO_ERROR;
}

fs_perm_t nx_fs_get_perms(fs_fd_t fd) {
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  if (!file) {
    return (fs_perm_t) FS_ERR_INVALID_FD;
  }

  return file->perms;
}

fs_err_t nx_fs_set_perms(fs_fd_t fd, fs_perm_t perms) {
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  file->perms = perms;

  return FS_ERR_NO_ERROR;
}

/* Delete and close the given file. */
fs_err_t nx_fs_unlink(fs_fd_t fd) {
  fs_file_t *file;
  U32 page, end;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  /* Remove file marker and potential in-file marker-alike. */
  end = file->origin + nx_fs_get_file_page_count(file->size);
  for (page = file->origin; page < end; page++) {
    if (nx_fs_page_has_magic(page)) {
      if (!nx__efc_erase_page(page, 0)) {
        return FS_ERR_FLASH_ERROR;
      }
    }
  }

  file->used = FALSE;
  return FS_ERR_NO_ERROR;
}

fs_err_t nx_fs_soft_format(void) {
  U32 nulldata[EFC_PAGE_WORDS] = {0};
  U32 i, j;

  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);
      size_t size, npages;

      size = nx_fs_get_file_size_from_metadata(metadata);
      npages = nx_fs_get_file_page_count(size);
      nx_display_string("erasing ");
      nx_display_uint(npages);
      nx_display_end_line();

      for (j=i; j<i+npages && j<FS_PAGE_END; j++) {
        nx_display_string("wiping ");
        nx_display_uint(j);
        nx_display_end_line();

        nx__efc_write_page(nulldata, j);
      }

      i += npages - 1;
    }
  }

  return FS_ERR_NO_ERROR;
}

/* Seek to the given position in the file.
 */
fs_err_t nx_fs_seek(fs_fd_t fd, size_t position) {
  fs_file_t *file;
  U32 page;
  U32 pos;

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  if (position > file->size) {
    return FS_ERR_INCORRECT_SEEK;
  }

  position += FS_FILE_METADATA_BYTES;

  page = file->origin + position / EFC_PAGE_BYTES;
  pos = position % EFC_PAGE_BYTES;

  if (page != file->rbuf.page) {
    nx__efc_read_page(page, file->rbuf.data.raw);
    file->rbuf.page = page;
  }

  file->rbuf.pos = pos;

  /* Same for wbuf ? */
  return FS_ERR_NO_ERROR;
}

void nx_fs_get_occupation(U32 *files, U32 *used, U32 *free_pages,
    U32 *wasted) {
  U32 _files = 0, _used = 0, _free_pages = 0, _wasted = 0;
  U32 i;

  for (i=FS_PAGE_START; i<FS_PAGE_END; i++) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);
      size_t size;
      U32 pages;

      size = nx_fs_get_file_size_from_metadata(metadata);
      pages = nx_fs_get_file_page_count(size);

      _files++;
      _used += size;
      _wasted += pages * EFC_PAGE_BYTES - size - FS_FILE_METADATA_BYTES;

      i += pages - 1;
    } else {
      _free_pages++;
    }
  }

  if (files) {
    *files = _files;
  }

  if (used) {
    *used = _used;
  }

  if (free_pages) {
    *free_pages = _free_pages;
  }

  if (wasted) {
    *wasted = _wasted;
  }
}

static fs_err_t nx_fs_find_next_hole(U32 start, U32 *origin) {
  U32 i;

  i = start;
  while (i < FS_PAGE_END) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);
      i += nx_fs_get_file_page_count(nx_fs_get_file_size_from_metadata(metadata));
    } else {
      *origin = i;
      return FS_ERR_NO_ERROR;
    }
  }

  return FS_ERR_FILE_NOT_FOUND;
}

void nx_fs_dump(void) {
  U32 i = FS_PAGE_START, origin = 0;
  union U32tochar nameconv;

  while (nx_fs_find_next_origin(i, &origin) == FS_ERR_NO_ERROR) {
    volatile U32 *metadata = &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS]);
    size_t npages = nx_fs_get_file_page_count(
      nx_fs_get_file_size_from_metadata(metadata));

    memcpy(nameconv.integers,
           (void *)(metadata + FS_FILENAME_OFFSET),
           FS_FILENAME_LENGTH);

    nx_display_uint(origin);
    nx_display_string(":");
    nx_display_string(nameconv.chars);
    nx_display_string(":");
    nx_display_uint(npages);
    nx_display_end_line();

    i = origin + npages;
  }

  nx_display_string("--");
  nx_display_uint(i);
  nx_display_string("--\n");
}

/* Defrag functions. */

/* Simple defragmentation function: tries to concatenate data blocks at
 * the beginning of the flash.
 *
 * To acheive this, we iterate through every "hole" in the flash (empty
 * spage), and try to find a block (contiguous set of files) that best
 * matches the hole. If no block smaller or equal than the hole size is
 * found, pull backwards the next block to fill the hole, if block there
 * is.
 */
fs_err_t nx_fs_defrag_simple_zone(U32 zone_start, U32 zone_end) {
  U32 next_hole, next_file, first_next_file, hole_length = 0, end_of_block;
  U32 best_block_origin =0, best_block_size=0, block_size = 0;
  U32 i;
  fs_err_t err = FS_ERR_NO_ERROR;

  NX_ASSERT(zone_start >= FS_PAGE_START);
  NX_ASSERT(zone_end <= FS_PAGE_END);
  NX_ASSERT(zone_start <= zone_end);

  i = zone_start;
  nx_display_string("<<  ");
  nx_display_uint(i);
  nx_display_end_line();

  while (i < zone_end) {
    /* Find the next hole, and its size. */
    err = nx_fs_find_next_hole(i, &next_hole);
    if (err != FS_ERR_NO_ERROR) {
      /* No free blocks left on flash : nothing else to do */
      return FS_ERR_NO_ERROR;
    }

    err = nx_fs_find_next_origin(next_hole, &next_file);
    if (err != FS_ERR_NO_ERROR) {
      /* No files found after : nothing else to do */
      return FS_ERR_NO_ERROR;
    }

    first_next_file = next_file;
    hole_length = next_file - next_hole;

    NX_ASSERT(hole_length > 0);

    /* Search for the best block to move. */
    do {
      if (nx_fs_find_next_hole(next_file, &end_of_block) != FS_ERR_NO_ERROR) {
        /* No free space after */
        break;
      }

      block_size = end_of_block - next_file;
      if (block_size < hole_length && block_size > best_block_size) {
        best_block_size = block_size;
        best_block_origin = next_file;
      }

      if (block_size != hole_length &&
          nx_fs_find_next_origin(end_of_block, &next_file) != FS_ERR_NO_ERROR) {
        break;
      }
    } while (next_file < zone_end && block_size != hole_length);

    /* Move the block to fill the hole and move the search start
     * position after the block.
     */

    /* First case: the block found exactly matches the hole. */
    if (block_size == hole_length) {
      err = nx_fs_move_region(next_file, next_hole, block_size);
      if (err != FS_ERR_NO_ERROR) {
        return err;
      }

      i = next_hole + block_size;
    }

    /* Else, if a best match has been found, move it. */
    else if (best_block_origin != 0) {
      nx_display_string("bmatch\n");
      nx_display_uint(best_block_origin);
      nx_display_string(">");
      nx_display_uint(next_hole);
      nx_display_string(" ");
      nx_display_uint(best_block_size);
      nx_display_end_line();

      err = nx_fs_move_region(best_block_origin, next_hole, best_block_size);
      if (err != FS_ERR_NO_ERROR) {
        return err;
      }

      i = next_hole + best_block_size;
      nx_display_string("after ");
      nx_display_uint(i);
      nx_display_end_line();
      return FS_ERR_NO_ERROR;
    }

    /* Otherwise, if no matching block was found, pull the next block
     * backwards to fill the hole.
     */
    else {
      err = nx_fs_find_next_hole(first_next_file, &end_of_block);
      if (err != FS_ERR_NO_ERROR) {
        end_of_block = zone_end;
      }

      nx_display_string("pull\n");

      err = nx_fs_move_region(first_next_file, next_hole,
                              end_of_block - first_next_file);
      if (err != FS_ERR_NO_ERROR) {
        return err;
      }

      i = next_hole + end_of_block - first_next_file;
    }
  }

  return FS_ERR_NO_ERROR;
}

/* Simple defragmentation of the whole flash. */
fs_err_t nx_fs_defrag_simple(void) {
  return nx_fs_defrag_simple_zone(FS_PAGE_START, FS_PAGE_END);
}

fs_err_t nx_fs_defrag_for_file_by_name(char *name) {
  U32 origin;
  fs_err_t err;

  err = nx_fs_find_file_origin(name, &origin);
  if (err == FS_ERR_NO_ERROR) {
    return nx_fs_defrag_for_file_by_origin(origin);
  }

  return FS_ERR_FILE_NOT_FOUND;
}

static fs_err_t nx_fs_swap_regions(U32 start1, U32 dest1, U32 len1,
                                   U32 start2, U32 len2) {
  U32 data[EFC_PAGE_WORDS] = {0};
  fs_err_t err;
  U32 i, j;

  NX_ASSERT(len2 <= len1);

  nx_display_string("swap\n");
  nx_display_uint(start1);
  nx_display_string("-");
  nx_display_uint(dest1);
  nx_display_string("-");
  nx_display_uint(len1);
  nx_display_end_line();

  nx_display_uint(start2);
  nx_display_string("-");
  nx_display_uint(len2);
  nx_display_end_line();

  for (i=0, j=0; i<len1; i++, j++) {
    if (j < len2) {
      nx__efc_read_page(start2 + j, data);
      nx__efc_erase_page(start2 + j, 0);
    }

    err = nx_fs_move_region(start1 + i, dest1 + i, 1);
    if (err != FS_ERR_NO_ERROR) {
      return err;
    }

    if (j < len2 && !nx__efc_write_page(data, start1 + j)) {
      return FS_ERR_FLASH_ERROR;
    }
  }

  return FS_ERR_NO_ERROR;
}

fs_err_t nx_fs_defrag_for_file_by_origin(U32 origin) {
  U32 next_origin=0, next_hole, last_origin, last_npages, npages;
  volatile U32 *metadata;
  fs_err_t err;

  /* First, trivial case: the file is already at the end of the flash.
   * If it still has free space after him, job's done. Otherwise, launch
   * a defrag simple.
   */
  err = nx_fs_find_last_origin(&last_origin);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }

  metadata = &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS]);
  npages = nx_fs_get_file_page_count(
    nx_fs_get_file_size_from_metadata(metadata));

  metadata = &(FLASH_BASE_PTR[last_origin*EFC_PAGE_WORDS]);
  last_npages = nx_fs_get_file_page_count(
    nx_fs_get_file_size_from_metadata(metadata));

  if (origin == last_origin) {
    nx_display_string("case1\n");
    return nx_fs_defrag_simple();
  }

  /* Second case: we can move our file to the end of the flash. If this
   * is possible, move the file and call nx_fs_defrag_simple().
   */
  if (FS_PAGE_END - (last_origin + last_npages) > npages) {
    nx_display_string("case2\n");

    err = nx_fs_move_region(origin, last_origin + last_npages, npages);
    if (err != FS_ERR_NO_ERROR) {
      return err;
    }

    return nx_fs_defrag_simple();
  }

  /* Third case, trickier: if we can *swap* the file with the last one
   * of the flash, do it and we're back into case 2).
   */
  do {
    if (nx_fs_find_next_hole(origin, &next_hole) != FS_ERR_NO_ERROR) {
      break;
    }

    if (nx_fs_find_next_origin(next_hole, &next_origin) !=
      FS_ERR_NO_ERROR) {
      break;
    }
  } while (next_origin != last_origin);

  if (next_origin == last_origin && npages <= FS_PAGE_END - next_hole &&
    last_npages <= npages) {
    nx_display_string("case3\n");

    /* Swap our file with the last one, if they fit. */
    err = nx_fs_swap_regions(origin, next_hole, npages,
                             last_origin, last_npages);
    if (err != FS_ERR_NO_ERROR) {
      return err;
    }

    return nx_fs_defrag_simple();
  }

  return FS_ERR_NO_SPACE_LEFT_ON_DEVICE;
}

static fs_err_t nx_fs_defrag_pull_file_to(U32 origin, U32 dest) {
  volatile U32 *metadata;
  size_t size;

  metadata = &(FLASH_BASE_PTR[origin*EFC_PAGE_WORDS]);
  size = nx_fs_get_file_size_from_metadata(metadata);

  return nx_fs_move_region(origin, dest,
    nx_fs_get_file_page_count(size));
}

static U32 nx_fs_defrag_get_mean_space(void) {
  U32 files, used, free_pages, wasted;

  /* Get the number of files and freepages. */
  nx_fs_get_occupation(&files, &used, &free_pages, &wasted);
  if (!files) {
    return 0;
  }

  return free_pages / files;
}

fs_err_t nx_fs_defrag_best_overall(void) {
  U32 next_origin = 0, mean_space_per_file = 0, i;
  fs_err_t err;

  mean_space_per_file = nx_fs_defrag_get_mean_space();

  /* Nothing to do here, move on */
  if (!mean_space_per_file) {
    return nx_fs_defrag_simple();
  }

  i = FS_PAGE_START;

  /* First, pull the first block at the beginning of the flash. */
  if (nx_fs_find_next_origin(i, &next_origin) != FS_ERR_NO_ERROR) {
    return FS_ERR_NO_ERROR;
  }

  err = nx_fs_defrag_pull_file_to(next_origin, i);
  if (err != FS_ERR_NO_ERROR) {
    return err;
  }

  nx_display_string("\nMSPF: ");
  nx_display_uint(mean_space_per_file);
  nx_display_end_line();

  /* Then, iterate on all files to set a proper space after them. */
  while (i < FS_PAGE_END) {
    if (nx_fs_page_has_magic(i)) {
      volatile U32 *metadata;
      size_t size, npages, hole_size;

      metadata = &(FLASH_BASE_PTR[i*EFC_PAGE_WORDS]);
      size = nx_fs_get_file_size_from_metadata(metadata);
      npages = nx_fs_get_file_page_count(size);

      /* No file left after this one, job's done. */
      if (nx_fs_find_next_origin(i + npages, &next_origin) != FS_ERR_NO_ERROR) {
        return FS_ERR_NO_ERROR;
      }

      hole_size = next_origin - (i + npages);
      i += npages + mean_space_per_file;

      /* First, trivial case: the hole size equals mean_space_per_file.
       * We then have nothing to do and can proceed to the next file.
       */
      if (hole_size == mean_space_per_file) {
        continue;
      }

      /* Second case: the hole size is greater than what we want: pull
       * the next file at (beginning of the hole + mean_space_per_file).
       */
      else if (hole_size > mean_space_per_file) {
        err = nx_fs_defrag_pull_file_to(next_origin, i);
        if (err != FS_ERR_NO_ERROR) {
          return err;
        }
      }

      /* Last case: the hole is smaller than what we need: try to move
       * what's after this file a bit forward to make room for
       * mean_space_per_file pages.
       */
      else {
        U32 next_hole = 0, hole_end = 0;
        size_t next_hole_size;

        if (nx_fs_find_next_hole(next_origin, &next_hole) !=
          FS_ERR_NO_ERROR) {
          /* The block goes all the way to the end of the flash: we
           * can't do anything anymore, just leave.
           */
          return FS_ERR_NO_ERROR;
        }

        if (nx_fs_find_next_origin(next_hole, &hole_end) != FS_ERR_NO_ERROR) {
          hole_end = FS_PAGE_END-1;
        }

        next_hole_size = hole_end - next_hole;

        /* First sub-case: we have enough space after this block to
         * extend our hole up to mean_space_per_file pages.
         */
        if (next_hole_size >= mean_space_per_file - hole_size) {
          /* Move the block [next_origin ; next_hole]
           * (mean_space_per_file - hole_size) pages to the right.
           */
          err = nx_fs_move_region(next_origin,
            next_origin + (mean_space_per_file - hole_size),
            next_hole - next_origin);
          if (err != FS_ERR_NO_ERROR) {
            return err;
          }
        }

        /* Otherwise, we need to make room for mean_space_per_file -
         * hole_size pages in one way or another.
         */
        else {
          /* First, we'll try to move the first file of the obstructing
           * block somewhere else on the right end side of the flash.
           */
          U32 file_origin = next_origin;
          size_t file_size = next_hole - next_origin;
          fs_err_t search_err;

          while ((search_err = nx_fs_find_next_hole(next_origin, &next_hole)) ==
            FS_ERR_NO_ERROR) {

            if (nx_fs_find_next_origin(next_hole, &next_origin) != FS_ERR_NO_ERROR) {
              next_origin = FS_PAGE_END - 1;
            }

            if (next_origin - next_hole >= file_size) {
              /* We found a big enough hole, move our file and stop
               * searching.
               */
              fs_err_t move_err = nx_fs_move_region(file_origin, next_hole, file_size);
              if (move_err != FS_ERR_NO_ERROR) {
                return err;
              }

              break;
            }

            /* Avoid looping on the last page. */
            if (next_origin == FS_PAGE_END - 1) {
              next_origin++;
            }
          }

          /* If the file was not moved, try to move the whole block
           * (including holes smaller than what we need, i.e.
           * mean_space_per_file - hole_size).
           */
          if (search_err == FS_ERR_FILE_NOT_FOUND) {
            next_origin = file_origin;

            while ((search_err = nx_fs_find_next_hole(next_origin, &next_hole))
              == FS_ERR_NO_ERROR) {

              if (nx_fs_find_next_origin(next_hole, &next_origin) != FS_ERR_NO_ERROR) {
                next_origin = FS_PAGE_END - 1;
              }

              if (next_origin - next_hole >= mean_space_per_file - hole_size) {
                /* With this hole, we'll be able to move this block far
                 * enough to the right to complete the hole we want to
                 * enl4rg3 up to mean_space_per_file.
                 */
                break;
              }

              if (next_origin == FS_PAGE_END - 1) {
                next_origin++;
              }
            }

            /* In the case we were not able to find enough room to move
             * this block away, we can end the defragmentation process
             * as this case is becoming too expensive.
             */
            if (search_err != FS_ERR_NO_ERROR) {
              return FS_ERR_NO_SPACE_LEFT_ON_DEVICE;
            }

            /* Move this whole block [file_origin ; next_hole] of
             * (mean_space_per_file - hole_size) pages to the right.
             */
            err = nx_fs_move_region(file_origin,
              file_origin + (mean_space_per_file - hole_size),
              next_hole - file_origin);
            if (err != FS_ERR_NO_ERROR) {
              return err;
            }
          }

          /* End of the 2nd sub-case of the last case (meh.). */
          i -= mean_space_per_file + npages;
        }
      }
    }
  }

  return FS_ERR_NO_ERROR;
}

