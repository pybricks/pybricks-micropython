/** @file dump.h
 *  @brief Data dumping utility.
 *
 * Data dumping utility for the NXT baseplate and application kernels.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_DUMP_H__
#define __NXOS_DUMP_H__

#include "base/types.h"

/** @addtogroup kernel */
/*@{*/

/** @defgroup dump Data dumping utility.
 */
/*@{*/

/** Initialize the data dumping system.
 *
 * @param ptr A start pointer in the userspace memory land. A NULL value
 * will tell the system to use __userspace_start__ as the memory area
 * starting point.
 */
void nx_dump_init(U8 *ptr);

/** Dump data to memory.
 *
 * @param data The data to store.
 * @param size The data size.
 *
 * @return This function will return TRUE if and only if the dumping system
 * has been initialized and if the dumping memory region has enough space for
 * the provided data.
 */
bool nx_dump_data(U8 *data, U32 size);

/** Dump a string to memory.
 *
 * @param str The string to store.
 *
 * @return Returns TRUE if the save succeeded. See nx_dump_data() for
 * more details.
 */
bool nx_dump_string(const char *str);

/** Dump a U8 as ASCII to memory.
 *
 * @param val The U8 value to store.
 *
 * @return Returns TRUE if the save succeeded. See nx_dump_data() for
 * more details.
 */
bool nx_dump_u8(U8 val);

/** Send the stored data via USB.
 */
void nx_dump_send_usb();

/*}@*/
/*}@*/

#endif /* __NXOS_DUMP_H__ */
