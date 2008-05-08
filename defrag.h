/** @file defrag.h
 *  @brief Flash file system defragmentation utilities.
 *
 * A defragmentation and optimization tool for the NxOS file system.
 */

/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DEFRAG_H__
#define __NXOS_BASE_DEFRAG_H__

#include "base/types.h"
#include "base/drivers/_efc.h"
#include "_fs.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup defragmentation Defragmentation utilities */
/*@{*/

typedef enum {
  DEFRAG_ERR_NO_ERROR,
  DEFRAG_ERR_NOT_ENOUGH_SPACE,
  DEFRAG_ERR_FLASH_ERROR,
} defrag_err_t;

defrag_err_t nx_defrag_simple(void);

defrag_err_t nx_defrag_for_file_by_name(char *name);

defrag_err_t nx_defrag_for_file_by_origin(U32 origin);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DEFRAG_H__ */

