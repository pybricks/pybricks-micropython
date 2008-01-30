/** @file _uart.h
 *  @brief UART driver for bluetooth communications.
 *
 * This driver only support the uart where the bluetooth chip is
 * connected => Only used by bt.(c|h)
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_UART_H__
#define __NXOS_BASE_DRIVERS_UART_H__

#include "base/types.h"

/** @addtogroup driverinternal */
/*@{*/

/** @defgroup uart UART driver
 *
 * The UART controller manages the link with the BlueCore Bluetooth
 * chip. It is for use exclusively by the Bluetooth driver.
 */
/*@{*/

/* Buffer size for UART messages. */
#define UART_BUFSIZE 128

/** Prototype for the UART read callback.
 *
 * The UART driver fires the callback with a @a buffer of length @a
 * packet_size of data to process.
 */
typedef void (*nx__uart_read_callback_t)(U8 *buffer, U32 packet_size);

/** Initialize the UART driver.
 *
 * @param callback The callback to fire when the UART receives data.
 */
void nx__uart_init(nx__uart_read_callback_t callback);

/**
 * @param callback new callback to use
 * @note if callback is null, interruptions are disabled and PDC is disabled
 */
void nx__uart_set_callback(nx__uart_read_callback_t callback);

/**
 * Set manually the PDC
 */
void nx__uart_read(U8 *buf, U32 length);

/**
 * Indicates how many bytes the PDC has already read from the UART
 */
U32 nx__uart_data_read(void);

/** Write @a lng bytes from @a data over the UART bus.
 *
 * @param data A pointer to the data to write.
 * @param lng The number of bytes to write.
 */
void nx__uart_write(const U8 *data, U32 lng);

/** Check if the UART can be written to.
 *
 * @return TRUE if the UART is idle and can be written to, else
 * FALSE.
 */
bool nx__uart_can_write(void);

/** Check if the UART is currently writing data.
 *
 * @return TRUE if the UART is busy writing, else FALSE.
 */
bool nx__uart_is_writing(void);

/*@}*/
/*@}*/

#endif
