/** @file tracing.h
 *  @brief In-memory data tracing facility.
 *
 * Data tracing utility for the NXT baseplate and application kernels.
 */

/* Copyright (c) 2007-2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_LIB_TRACING_TRACING_H__
#define __NXOS_BASE_LIB_TRACING_TRACING_H__

#include "base/types.h"

/** @addtogroup lib */
/*@{*/

/** @defgroup tracing Data tracer
 *
 * The data tracer provides a handy debugging facility. It serializes
 * data into a trace buffer, which can then be sent in bulk to a host
 * computer for analysis.
 *
 * As an example, in the past it was used to develop the software I2C
 * driver in the Baseplate. The tracer was used to record the bus
 * state at regular intervals, to visualize the progress of I2C
 * transactions as the driver was being debugged.
 */
/*@{*/

/** Initialize the data tracer.
 *
 * @param start Pointer to the start of the dump area.
 * @param size The amount of memory available for tracing.
 */
void nx_tracing_init(U8 *start, U32 size);

/** Add data to the trace.
 *
 * @param data The data to store.
 * @param size The data size.
 *
 * @note This function will cause an assertion failure if insufficient
 * space remains in the tracing buffer.
 */
void nx_tracing_add_data(const U8 *data, U32 size);

/** Add a string to the trace.
 *
 * @param str The string to store.
 *
 * @sa nx_tracing_add_data
 */
void nx_tracing_add_string(const char *str);

/** Add a character to the trace.
 *
 * @param val The character to store.
 *
 * @sa nx_tracing_add_data
 */
void nx_tracing_add_char(const char val);

/** Retrieve the trace buffer.
 *
 * @return The address of the start of the trace buffer.
 */
U8 *nx_tracing_get_start(void);

/** Get the size of the trace.
 *
 * @return The size of the recorded trace.
 */
U32 nx_tracing_get_size(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_LIB_TRACING_TRACING_H__ */
