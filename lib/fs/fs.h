/** @file fs.h
 *  @brief Flash file system.
 *
 * A flash-friendly file system for the NXT on-board flash memory.
 */

/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LIB_FS_H__
#define __NXOS_BASE_LIB_FS_H__

#include "base/types.h"
#include "base/drivers/_efc.h"

/** @addtogroup lib */
/*@{*/

/** @defgroup fs Flash file system
 *
 * A flash-friendly file system for the NXT on-board flash memory. This is a very simple
 * file system implementation, allowing most of the basic features expected from a file
 * system: open(), read(), write(), seek(), flush() and close(). Note that reading and
 * writing use two different pointers. A seek() will move both of them.
 *
 * The file system also tries to minimize stress on the flash by progressively moving files
 * needing more space towards the end of the flash medium. This relocation process happens
 * automatically and may make one write operation rather costly (in terms of time).
 *
 * For more information, refer to the file system design document.
 */
/*@{*/

/** File-system first page number. */
#define FS_PAGE_START 128

/** File-system last page number. */
#define FS_PAGE_END 1024

/** Maximum number of files that can be stored by the filesystem.
 * The lack of dynamic memory allocator makes this a hardcoded
 * limitation.
 */
#define FS_MAX_OPENED_FILES 8

/** Filename length, in U32s. */
#define FS_FILENAME_SIZE 8

/** Maximum allowed filename length (in bytes). */
#define FS_FILENAME_LENGTH (FS_FILENAME_SIZE * sizeof(U32))

/** File system errors. */
typedef enum {
  FS_ERR_NO_ERROR = 0,
  FS_ERR_NOT_FORMATTED,
  FS_ERR_FILE_NOT_FOUND,
  FS_ERR_FILE_ALREADY_EXISTS,
  FS_ERR_TOO_MANY_OPENED_FILES,
  FS_ERR_INVALID_FD,
  FS_ERR_END_OF_FILE,
  FS_ERR_UNSUPPORTED_MODE,
  FS_ERR_CORRUPTED_FILE,
  FS_ERR_FLASH_ERROR,
  FS_ERR_NO_SPACE_LEFT_ON_DEVICE,
  FS_ERR_INCORRECT_SEEK,
} fs_err_t;

/** File permission modes. */
typedef enum {
  FS_PERM_READONLY,
  FS_PERM_READWRITE,
  FS_PERM_EXECUTABLE,
} fs_perm_t;

/** File opening modes. */
typedef enum {
  FS_FILE_MODE_OPEN,
  FS_FILE_MODE_APPEND,
  FS_FILE_MODE_CREATE,
} fs_file_mode_t;

/** File I/O buffer. */
typedef struct {
  union {
    U32 raw[EFC_PAGE_WORDS];
    U8 bytes[EFC_PAGE_BYTES];
  } data;    /**< The buffer data, accessible in its raw (U32) form or byte
              * per byte.
              */
  U32 page;  /**< The flash page this buffer is related to. */
  U32 pos;   /**< In-data cursor. */
} fs_buffer_t;

/** File description structure, read from the file's metadata
 * (FS_FILE_METADATA_SIZE bytes). */
typedef struct {
  bool used;                     /**< Denotes fd usage. */
  char name[FS_FILENAME_LENGTH]; /**< The file name. */

  U32 origin;                    /**< File origin page on the flash */
  size_t size;                   /**< The file size. */

  fs_perm_t perms;               /**< File permissions. */

  fs_buffer_t rbuf;              /**< Read buffer. */
  fs_buffer_t wbuf;              /**< Write buffer. */
} fs_file_t;

/** File descriptor type. */
typedef U8 fs_fd_t;

/** Initializes the file system.
 *
 * @return An @a fs_err_t error describing the outcome of the operation.
 */
fs_err_t nx_fs_init(void);

/** Open a file.
 *
 * @param name The name of the file to open.
 * @param mode The requested file mode.
 * @param fd A pointer to the file descriptor to use.
 */
fs_err_t nx_fs_open(char *name, fs_file_mode_t mode, fs_fd_t *fd);

/** Get the file size.
 *
 * @param fd The file descriptor.
 * @return The file size as a @a size_t.
 */
size_t nx_fs_get_filesize(fs_fd_t fd);

/** Read one byte from a file.
 *
 * @param fd The descriptor for the file to read from.
 * @param byte A pointer to a U32 to write the read byte from.
 */
fs_err_t nx_fs_read(fs_fd_t fd, U8 *byte);

/** Write one byte to a file.
 *
 * @param fd The descriptor for the file to write to.
 * @param byte The byte to write to the file.
 * @return An @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_write(fs_fd_t fd, U8 byte);

/** Flush a file's write buffer. */
fs_err_t nx_fs_flush(fs_fd_t fd);

/** Close the file, flushing any data left to be written and sync
 * its metadata.
 *
 * @param fd The file descpriptor.
 * @return An @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_close(fs_fd_t fd);

/** Get file permissions.
 *
 * @return The current file permissions.
 */
fs_perm_t nx_fs_get_perms(fs_fd_t fd);

/** Set file permissions.
 *
 * @param fd The file descpriptor.
 * @param perms The new file permissions.
 * @return An @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_set_perms(fs_fd_t fd, fs_perm_t perms);

/** Delete and close the file.
 *
 * @param fd The file descpriptor.
 * @return An @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_unlink(fs_fd_t fd);

/** Soft format the flash by wiping all present files.
 */
fs_err_t nx_fs_soft_format(void);

/** Seek to a given position in a file.
 *
 * @param fd The file descpriptor.
 * @param position The position to seek to, in bytes.
 * @return An @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_seek(fs_fd_t fd, size_t position);

/** Compute file system occupation level and statistics. Values
 * are returned to the provided pointers, if they are not NULL.
 *
 * @param files The number of files.
 * @param used Bytes used.
 * @param free_pages The number of available pages.
 * @param wasted The bytes lost by files page aligment.
 */
void nx_fs_get_occupation(U32 *files, U32 *used, U32 *free_pages,
                          U32 *wasted);

/** Dumps the index of the filesystem as <page>:<filename>.
 */
void nx_fs_dump(void);

/** Perform a simple defragmentation of the flash filesystem on the
 * given zone of the flash.
 *
 * @param zone_start Beginning of the zone to defragment.
 * @param zone_end End of the zone.
 * @return A @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_defrag_simple_zone(U32 zone_start, U32 zone_end);

/** Perform a simple defragmentation of the flash filesystem.
 *
 * This type of defragmentation only tries to collate files towards
 * the beginning of the flash medium, maximizing free space at the end of
 * the flash.
 *
 * @return A @a fs_err_t describing the outcome of the operation.
 */
inline fs_err_t nx_fs_defrag_simple(void);

/** Perform a simple, file oriented defragmentation of the flash.
 *
 * This defragmentation type is similar to the simple one, but
 * with the objective of making subsequent writes to the given file
 * (by its name) faster by putting it at the end of the flash medium.
 *
 * @return A @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_defrag_for_file_by_name(char *name);

/** Same as @a nx_defrag_for_file_by_name but takes a file origin
 * instead. This function is called by @a nx_defrag_for_file_by_name
 * once it has found the requested file's origin.
 *
 * @return A @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_defrag_for_file_by_origin(U32 origin);

/** Tries to optimize the placement of the files on the filesystem
 * to make write operations faster for all files by putting as much
 * space as possible between each file and thus avoid a costy file
 * relocation.
 *
 * @return A @a fs_err_t describing the outcome of the operation.
 */
fs_err_t nx_fs_defrag_best_overall(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_FS_H__ */
