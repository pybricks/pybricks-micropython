/** @file _fs.h
 *  @brief Flash file system internal APIs.
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

#ifndef __NXOS_BASE__FS_H__
#define __NXOS_BASE__FS_H__

#include "base/types.h"
#include "fs.h"

/** @addtogroup kernelinternal */
/*@{*/

/** @name File system internals.
 *
 */
/*@{*/

#define FS_PAGE_START 128
#define FS_PAGE_END 1024

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

union U32tochar {
  char chars[FS_FILENAME_LENGTH];
  U32 integers[FS_FILENAME_SIZE];
};

/** FD-set. */
fs_file_t fdset[FS_MAX_OPENED_FILES];

/** Returns a file info structure given its file descriptor.
 *
 * @param fd The file descriptor (must be valid, as in pointing to
 * a opened file).
 * @return The file info structure if the @a fd is valid, or NULL
 * if it refers to a no longer opened or inexistant file.
 */
fs_file_t *nx__fs_get_file(fs_fd_t fd);

/** Determines if the given page contains a file origin marker.
 *
 * @param page The page number.
 */
inline bool nx__fs_page_has_magic(U32 page);

/** Find a file's origin on the file system by its name.
 *
 * @param name The file name.
 * @param origin A pointer to an U32 where the result will be stored.
 * @return An appropriate @a fs_err_t error code.
 */
fs_err_t nx__fs_find_file_origin(char *name, U32 *origin);

/** Finds the last file origin on the flash.
 *
 * @param origin A pointer to an U32 where the result will be stored.
 * @return An appropriate @a fs_err_t error code.
 */
fs_err_t nx__fs_find_last_origin(U32 *origin);

fs_err_t nx__fs_find_next_origin(U32 start, U32 *origin);

/** Compute the number of pages used by a file.
 *
 * @param size The file size, in bytes.
 * @return The page count.
 */
U32 nx__fs_get_file_page_count(size_t size);

/** Extract the file size (in bytes) from the file's metadata.
 *
 * @param metadata A pointer to the metadata (should be an U32 array).
 * @return The file size, in bytes.
 */
size_t nx__fs_get_file_size_from_metadata(volatile U32 *metadata);

/** Extract the file permissions from the file's metadata.
 *
 * @param metadata A pointer to the metadata (should be an U32 array).
 * @return The file permissions, defaulting to read-only.
 */
fs_perm_t nx__fs_get_file_perms_from_metadata(volatile U32 *metadata);

/** Serialize a file's metadata into a storable form. The @a metadata
 * pointer must be pre-allocated, and its first @a FS_FILE_METADATA_SIZE
 * U32s will be overwritten by the metadata.
 *
 * @param perms The file permissions.
 * @param name The file name (no more than 31 characters long, NUL excluded).
 * @param size The file size, in bytes.
 * @param metadata A pointer to U32 memory where the metadata will be stored.
 */
void nx__fs_create_metadata(fs_perm_t perms, char *name, size_t size,
                            U32 *metadata);
							
fs_err_t nx__fs_relocate_to_page(fs_file_t *file, U32 origin);

fs_err_t nx__fs_relocate(fs_file_t *file);

/*@}*/
/*@}*/


#endif /* __NXOS_BASE__FS_H__ */
