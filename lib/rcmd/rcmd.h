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
 * ...
 */
/*@{*/

typedef enum {
  RCMD_MOVE,
  RCMD_PRINT,
  RCMD_CLEAR,
  RCMD_PLAY,
  RCMD_EXEC,
} rcmd_command;

typedef enum {
  RCMD_ERR_NO_ERROR,
  RCMD_ERR_INVALID_ARGC,
  RCMD_ERR_INVALID_PARAMETER,
  RCMD_ERR_READ_ERROR,
  RCMD_ERR_N_ERRS,
} rcmd_err_t;

bool nx_rcmd_parse(char *file);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB_RCMD_H__ */
