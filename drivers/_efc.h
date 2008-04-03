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

/** Number of pages in the memory pane. WARNING: we must take into
 * consideration the fact that the kernel uses a couple of pages at
 * the bottom of the flash.
 */
#define EFC_PAGES 1024

/** Number of 32-bytes words per page (one page is 256 bytes long). */
#define EFC_PAGE_WORDS 64

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

/** Base address of the flash in the virtual address space. */
#define FLASH_BASE 0x00100000

/** A usable pointer to the base address of the flash. */
#define FLASH_BASE_PTR ((volatile U32 *)FLASH_BASE)

/** Initialize the flash subsystem. */
void nx__efc_init(void);

/** Write a page to the flash.
 *
 * @param data A pointer to the 64 U32s of the page.
 * @param page The page number in the flash memory.
 */
bool nx__efc_write_page(U32 *data, U32 page);

/** Read a page from the flash.
 *
 * Actually, just retrieve a pointer to the data.
 *
 * @param page The page number in the flash memory.
 * @param data A pointer to a 64 U32s long array for the page data.
 */
void nx__efc_read_page(U32 page, U32 *data);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS__EFC_H__ */

