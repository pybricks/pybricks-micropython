/** @file usb.h
 *  @brief Manage USB communications
 *
 * Driver for the NXT's USB port.
 *
 * This driver drives the onboard USB controller to make the NXT into
 * a functional USB 2.0 peripheral. Note that given the limitations of
 * the controller hardware, the brick cannot function as a host, only
 * as a peripheral.
 *
 * Here, the goal of this driver is to provide a simplified mechanism
 * to exchange data over usb :
 *  \li Endpoint 0 is used by the driver to control the usb connection
 *  \li Endpoint 1 is used for downloads (From the NXT PoV)
 *  \li Endpoint 2 is used for uploads   (From the NXT PoV)
 *
 * usb_send():
 *  \li If the end point is free, the function is non-blocking
 *  \li If there are already some data waiting for being sent, then this function will be blocking.
 *
 * usb_can_send():
 *  \li Allow to anticipate if usb_send() will be blocking or not.
 *  \li Returns 1 if no, 0 if yes.
 *
 * usb_has_data():
 *  \li Return the number of bytes waiting in the read buffer
 *
 * usb_get_buffer():
 *  \li Return a pointer to the user app buffer
 *  \li This pointer is constant
 *
 * usb_flush_buffer():
 *  \li Flush the current buffer
 *
 * usb_overflowed():
 *  \li Data were lost since the last flush
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_USB_H__
#define __NXOS_USB_H__

#include "base/types.h"

#define NX_USB_BUFFER_SIZE 64 /* usb packet size */

bool nx_usb_is_connected();

/**
 * If you need to know when your data has been sent, you can
 * use also this function
 */
bool nx_usb_can_send();

/**
 * send the specified data.
 * take care to not modify these data until usb_can_send() return true again
 */
void nx_usb_send(U8 *data, U32 length);

/**
 * return the number of bytes waiting in the input buffer
 */
U16 nx_usb_has_data();

/**
 * return a pointer to the user buffer
 * this buffer is always the same, so you can call this
 * function only once.
 */
volatile void *nx_usb_get_buffer();

/**
 * erase the user buffer with the content of the driver buffer
 * WARNING: Once you have handled the content of the user buffer
 *          and don't need it anymore, you must call this function,
 *          else, there will be a buffer overload.
 */
void nx_usb_flush_buffer();

/**
 * return true if the buffers were overloaded : It means your program wasn't
 * fast enought for reading data coming on the usb, and a packet has been lost.
 */
bool nx_usb_overloaded();


#endif

