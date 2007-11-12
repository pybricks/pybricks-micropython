/** @file usb.h
 *  @brief USB communication interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_USB_H__
#define __NXOS_BASE_DRIVERS_USB_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup usb USB communication
 *
 * This driver turns the NXT into a functional USB 2.0 peripheral.
 *
 * The device is configured with the following USB endpoints:
 *  @li Endpoint 0 is used by the driver to control the usb connection.
 *  @li Endpoint 1 is used for PC to NXT transfers.
 *  @li Endpoint 2 is used for NXT to PC transfers.
 *
 * @note Given the limitations of the controller hardware, the brick
 * cannot function as a host, only as a slave peripheral.
 */
/*@{*/

/** The size of an USB packet. */
#define NX_USB_BUFFER_SIZE 64

/** Check if the NXT is connected and configured on a USB bus.
 *
 * @return TRUE if the NXT is connected and configured, else FALSE.
 */
bool nx_usb_is_connected();

/** Check if a call to nx_usb_send() will block.
 *
 * @return TRUE if data can be sent, FALSE if the driver buffers are
 * saturated.
 */
bool nx_usb_can_send();

/** Send @a length bytes of @a data to the USB host.
 *
 * If there is already data buffered, this function may block. Use
 * nx_usb_can_send() to check for buffered data.
 *
 * @param data The data to send.
 * @param length The amount of data to send.
 */
void nx_usb_send(U8 *data, U32 length);

/**
 * Return TRUE when all the data has been sent to
 * the USB controller and that these data can be
 * freed/erased from the memory.
 */
bool nx_usb_data_sent();


/** Return the amount of data available in the input buffer.
 *
 * @return The amount of data available, in bytes.
 */
U32 nx_usb_has_data();

/** Return a pointer to the USB input buffer.
 *
 * @return A pointer to the input buffer.
 *
 * @note Despite the current prototype, this pointer is in fact
 * constant. You don't need to call this function multiple times.
 */
volatile void *nx_usb_get_buffer();

/** Release the contents of the USB input buffer.
 *
 * This signals the driver that it can receive more data. Call it as
 * soon as you have finished processing input.
 */
void nx_usb_flush_buffer();

/** Check if the USB driver overflowed on reading input.
 *
 * @return TRUE if too much data was received without
 * nx_usb_flush_buffer() being called, FALSE otherwise.
 */
bool nx_usb_overloaded();

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_USB_H__ */
