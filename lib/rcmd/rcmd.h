/** @file rcmd.h
 *  @brief Remote robot command library.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LIB_RCMD_H__
#define __NXOS_BASE_LIB_RCMD_H__

#include "base/types.h"

/** @addtogroup lib */
/*@{*/

/** @defgroup rcmd Remote robot command library
 *
 * The remote robot command library provides a small command parsing and
 * execution context to command a robot based on the NXT brick running NxOS.
 * Given the name of a file on the file system, or directly commands one
 * after an other, it will interpret these commands to control the bot.
 */
/*@{*/

/** Maximum command tokens. */
#define RCMD_MAX_TOKENS 4

/** Maximum length of a token (command argument). */
#define RCMD_MAX_TOKEN_LENGTH 32

/** Maximum length of one line. */
#define RCMD_BUF_LEN 256

/** Token separator character. */
#define RCMD_TOKEN_SEPARATOR ' '

/** Commented line mark character. */
#define RCMD_COMMENT_CHAR '#'

/** Recognized commands. */
typedef enum {
  RCMD_CMD_MOVE,
  RCMD_CMD_PRINT,
  RCMD_CMD_CLEAR,
  RCMD_CMD_PLAY,
  RCMD_CMD_EXEC,
  RCMD_CMD_WAIT,
} rcmd_command;

/** Error codes. */
typedef enum {
  RCMD_ERR_NO_ERROR,
  RCMD_ERR_INCORRECT_ARGC,
  RCMD_ERR_INVALID_PARAMETER,
  RCMD_ERR_READ_ERROR,
  RCMD_ERR_END_OF_FILE,
  RCMD_ERR_COMMAND_NOT_FOUND,
  RCMD_ERR_N_ERRS,
} rcmd_err_t;

/** Execute the given line.
 *
 * @param line The line to execute.
 * @return An appropriate @a rcmd_err_t error code.
 */
rcmd_err_t nx_rcmd_do(const char *line);

/** Parse the given file and execute each line.
 *
 * @param file The name of the file to parse and execute.
 */
void nx_rcmd_parse(char *file);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB_RCMD_H__ */
