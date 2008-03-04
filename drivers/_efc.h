/** @file _efc.h
 *  @brief Embedded Flash Controller driver.
 *
 * NXT on-board flash controller driver.
 */

/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS__EFC_H__
#define __NXOS_BASE_DRIVERS__EFC_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup efcinternal Embedded Flash Controller driver
 *
 * The flash driver provides a software interface to the on-board 
 * flash memory through the embedded flash controller.
 */
/*@{*/

/** Number of pages in the memory pane. */
#define EFC_PAGES 1024

/** Size in bytes of one page. */
#define EFC_PAGE_SIZE 256

/** Number of lock regions. */
#define EFC_LOCK_REGIONS 16

/** Write protection key. */
#define EFC_WRITE_KEY 0x5A

/** Embedded Flash Conroller commands. */
typedef enum {
    EFC_CMD_WP = 0x01,
    EFC_CMD_SLB,
    EFC_CMD_WPL,
    EFC_CMD_CLB,
    EFC_CMD_EA = 0x08,
    EFC_CMD_SGPB = 0x0B,
    EFC_CMD_CGPB = 0x0D,
    EFC_CMD_SSB = 0x0F,
} efc_cmd;


/** Initialize the flash subsystem. */
void nx__efc_init(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__EFC_H__ */

