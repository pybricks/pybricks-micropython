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
#include "base/fs.h"
#include "base/drivers/_efc.h"

/** Magic marker. */
#define FS_FILE_ORIGIN_MARKER 0x42

/** File metadata size, in U32s. */
#define FS_FILE_METADATA_SIZE 10

/** File metadata size, in bytes. */
#define FS_FILE_METADATA_BYTES (FS_FILE_METADATA_SIZE * sizeof(U32))

#define FS_FILE_ORIGIN_MASK 0xFF000000
#define FS_FILE_PERMS_MASK 0x00F00000
#define FS_FILE_SIZE_MASK 0x000FFFFF

#define FS_FILE_PERM_MASK_READWRITE (1 << 0)
#define FS_FILE_PERM_MASK_EXECUTABLE (1 << 1)

#define FS_FILENAME_OFFSET 2

/** U32 <-> char conversion union for filenames. */
union U32tochar {
  char chars[FS_FILENAME_LENGTH];
  U32 integers[FS_FILENAME_SIZE];
};

/** FD-set. */
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
  U32 data;

  NX_ASSERT(source < EFC_PAGES);
  NX_ASSERT(dest < EFC_PAGES);
  NX_ASSERT(len < EFC_PAGES);
  NX_ASSERT(dest - source <= len);

  while (len--) {
    data = FLASH_BASE_PTR[source*EFC_PAGE_WORDS];
    if (!nx__efc_write_page(&data, dest)) {
      return FS_ERR_FLASH_ERROR;
    }

    /* TODO: erase the source page ? */

    source++;
    dest++;
  }

  return FS_ERR_NO_ERROR;
}

/* Relocate the given file to @a origin.
 */
static fs_err_t nx_fs_relocate_to_page(fs_file_t *file, U32 origin) {
  U32 page_data[EFC_PAGE_WORDS], null_data[EFC_PAGE_WORDS] = {0};
  U32 diff, i;
  size_t size;

  diff = origin - file->origin;

  /* Move the file's data. */
  size = nx_fs_get_file_page_count(file->size);

  /* TODO: use nx_fs_move_region? */
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
  fs_file_t *file;

  file = nx_fs_get_file(fd);
  NX_ASSERT(file != NULL);

  file->origin = origin;
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
    return FS_ERR_INVALID_FD;
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
  U32 erase[EFC_PAGE_WORDS] = {0};

  file = nx_fs_get_file(fd);
  if (!file) {
    return FS_ERR_INVALID_FD;
  }

  /* Remove file marker and potential in-file marker-alike. */
  end = file->origin + nx_fs_get_file_page_count(file->size);
  for (page = file->origin; page < end; page++) {
    if (nx_fs_page_has_magic(page)) {
      /* Erase marker. */
      if (!nx__efc_write_page(erase, page)) {
        return FS_ERR_FLASH_ERROR;
      }
    }
  }

  file->used = FALSE;
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

void nx_fs_get_occupation(U16 *files, U32 *used, U32 *free_pages,
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

/** Defrag functions. */

fs_err_t nx_fs_defrag_simple(void) {
  return FS_ERR_NO_ERROR;
}

fs_err_t nx_fs_defrag_for_file_by_name(char *name) {
  U32 origin;
  fs_err_t err;

  err = nx_fs_find_file_origin(name, &origin);
  if (err == FS_ERR_NO_ERROR) {
    return nx_fs_defrag_for_file_by_origin(origin);
  }

  /* Fall back to simple defrag. */
  return nx_fs_defrag_simple();
}

fs_err_t nx_fs_defrag_for_file_by_origin(U32 origin) {
  nx_fs_move_region(origin, 1, 1); // TBR

  return FS_ERR_NO_ERROR;
}

fs_err_t nx_fs_defrag_best_overall(void) {
  return FS_ERR_NO_ERROR;
}

