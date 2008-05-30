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

/** @addtogroup libinternal */
/*@{*/

/** @defgroup rcmdinternal Remote robot command library
 *
 * Internal helper functions and actuators for the remote robot control
 * library.
 */
/*@{*/

/** Command definition. */
typedef struct {
  char *name;
  int argc;
  
  rcmd_err_t (* actuator)(const char*);
} rcmd_command_def;

/** Finds the appropriate command from the commands definition array
 * by matching on the command name.
 *
 * @param line The line to parse.
 * @param command A pointer to return the found command, if any.
 * @return An appropriate @a rcmd_err_t error code.
 */
rcmd_err_t nx__rcmd_find_command(const char *line, rcmd_command_def *command);

rcmd_err_t nx__rcmd_move(const char *line);
rcmd_err_t nx__rcmd_print(const char *line);
rcmd_err_t nx__rcmd_clear(const char *line);
rcmd_err_t nx__rcmd_play(const char *line);
rcmd_err_t nx__rcmd_exec(const char *line);

/** Read one line from the given file.
 *
 * @param fd The file descriptor of the file to read from.
 * @param line A pointer to a pre-allocated buffer where the data will be
 * stored.
 * @return An appropriate @a rcmd_err_t error code.
 */
rcmd_err_t nx__rcmd_readline(fs_fd_t fd, char *line);

void nx__rcmd_tokenize(const char *line, char sep, int *argc,
                       char argv[][RCMD_MAX_TOKEN_LENGTH]);

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
