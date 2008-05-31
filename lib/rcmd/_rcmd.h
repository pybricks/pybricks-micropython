/** @file _rcmd.h
 *  @brief Remote robot command library private header.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LIB__RCMD_H__
#define __NXOS_BASE_LIB__RCMD_H__

#include "base/types.h"
#include "base/fs.h"
#include "base/lib/rcmd/rcmd.h"

/** @addtogroup lib */
/*@{*/

/** @addtogroup rcmd */
/*@{*/

/** Command definition. */
typedef struct {
  char *name;  /**< Command name. */
  int argc;    /**< Number of arguments expected by this command. */

  /** Command actuator. */
  rcmd_err_t (* actuator)(char*);
} rcmd_command_def;

/** Finds the appropriate command from the commands definition array
 * by matching on the command name.
 *
 * @param line The line to parse.
 * @param command A pointer to return the found command, if any.
 * @return An appropriate @a rcmd_err_t error code.
 */
rcmd_err_t nx__rcmd_find_command(const char *line, rcmd_command_def *command);

rcmd_err_t nx__rcmd_move(char *line);
rcmd_err_t nx__rcmd_print(char *line);
rcmd_err_t nx__rcmd_clear(char *line);
rcmd_err_t nx__rcmd_play(char *line);
rcmd_err_t nx__rcmd_exec(char *line);
rcmd_err_t nx__rcmd_wait(char *line);

/** Read one line from the given file.
 *
 * @param fd The file descriptor of the file to read from.
 * @param line A pointer to a pre-allocated buffer where the data will be
 * stored.
 * @return An appropriate @a rcmd_err_t error code.
 */
rcmd_err_t nx__rcmd_readline(fs_fd_t fd, char *line);

void nx__rcmd_tokenize(char *line, char sep, int *ntokens, int *indices);

/** Display an error from its error code.
 *
 * Prints a comprehensive message when an error happens parsing @a filename.
 *
 * @param err The error code to display.
 * @param filename The name of the file this error happened into.
 * @param line The number of the line containing the error.
 */
void nx__rcmd_error(rcmd_err_t err, char *filename, int line);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB__RCMD_H__ */
