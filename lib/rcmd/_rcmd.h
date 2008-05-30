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
 * ...
 */
/*@{*/

#define RCMD_MAX_CMD_LENGTH 8

#define RCMD_MAX_ARGS 4
#define RCMD_MAX_TOKEN_LENGTH 32

#define RCMD_BUF_LEN 256
#define RCMD_TOKEN_SEPARATOR ' '

rcmd_err_t nx__rcmd_move(const char *line);
rcmd_err_t nx__rcmd_print(const char *line);
rcmd_err_t nx__rcmd_clear(const char *line);
rcmd_err_t nx__rcmd_play(const char *line);
rcmd_err_t nx__rcmd_exec(const char *line);

typedef struct {
  char *name;
  int argc;
  
  rcmd_err_t (* actuator)(const char*);
} rcmd_command_def;

extern rcmd_command_def rcmd_commands[];

rcmd_err_t nx__rcmd_readline(fs_fd_t fd, char *line);
void nx__rcmd_tokenize(const char *line, char sep, int *argc,
                       char argv[][RCMD_MAX_TOKEN_LENGTH]);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB__RCMD_H__ */
