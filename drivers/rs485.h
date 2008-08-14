/** @file rs485.h
 *  @brief RS485 Transmission support
 */

/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 *
 */

#ifndef __NXOS_BASE_DRIVERS_RS485_H__
#define __NXOS_BASE_DRIVERS_RS485_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup rs485 RS485 interface driver.
 *
 * The RS485 driver allows communication with other RS485 compatible
 * devices using sensor port number 4.
 */
/*@{*/

/** Rs485 driver baud rates */
typedef enum {
  RS485_BR_9600   = 9600,
  RS485_BR_19200  = 19200,
  RS485_BR_38400  = 38400,
  RS485_BR_57600  = 57600,
  RS485_BR_76800  = 76800,
  RS485_BR_115200 = 115200
} nx_rs485_baudrate_t;

/** Enables the transmission over the RS485 interface
 *
 * @warning The RS485 driver should only be used if sensor port 4 has not
 * already been configured as a normal sensor port.
 */
void nx_rs485_init(void);

/** Shuts down the peripheral
 *
 * @note Once the RS485 interface is shut down, port 4 may be used as a
 * normal sensor port again.
 */
bool nx_rs485_stop(void);

/** Asynchronously transmit data on the RS485 bus.
 *
 * The function returns as soon as the transmission is started. The callback
 * passed in is invoked when the transmission is completed.
 *
 * @param buffer The buffer containing the data to be sent.
 * @param buflen Length of the buffer.
 * @param callback The callback to activate when transfer is completed.
 * @return FALSE if the device is busy (the request is simply ignored) and
 *         TRUE otherwise.
 *
 * @warning The contents of the buffer passed to this function should
 * not be changed until transmission is complete.
 *
 * @note The callback executes in an interrupt handler context.
 */
bool nx_rs485_send(U8 *buffer, U32 buflen, nx_closure_t callback);

/** Asynchronously receives data from the RS485 bus.
 *
 * The function returns as soon as the reception is started. The callback
 * passed in is invoked when the reception is completed.
 *
 * @param buffer The buffer containing the data to be sent.
 * @param buflen Length of the buffer.
 * @param callback The callback to activate when transfer is completed.
 * @return FALSE if the device is busy (the request is simply ignored) and
 *         TRUE otherwise.
 *
 * @warning The contents of the buffer passed to this function should not
 * be changed until reception is complete.
 *
 * @note The callback executes in an interrupt handler context.
 */
bool nx_rs485_recv(U8 *buffer, U32 buflen, nx_closure_t callback);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_RS485_H__ */

