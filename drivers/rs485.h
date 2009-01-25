/** @file rs485.h
 *  @brief RS485 Transmission support
 */

/* Copyright (c) 2008,2009 the NxOS developers
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
  RS485_BR_9600   = 9600, /**< 9600 baud */
  RS485_BR_19200  = 19200, /**< 19200 baud */
  RS485_BR_38400  = 38400, /**< 38400 baud */
  RS485_BR_57600  = 57600, /**< 57600 baud */
  RS485_BR_76800  = 76800, /**< 76800 baud */
  RS485_BR_115200 = 115200 /**< 115200 baud */
} nx_rs485_baudrate_t;

typedef enum {
  RS485_SUCCESS = 0, /**< Operation succeded */
  RS485_TIMEOUT, /**< Time exceeded */
  RS485_OVERRUN, /**< Overrun error */
  RS485_FRAMING, /**< Framing error (wrong baudrate) */
  RS485_PARITY, /**< Parity check failure */
  RS485_ABORT /**< Operation aborted */
} nx_rs485_error_t;

/** Default settings for the serial interface
 * - Clock = MCK
 * - 8-bit Data
 * - No Parity
 * - Oversampling
 * - 1 Stop Bit
 * - Most significative bit first
 */
#define DEFAULT_USART_MODEREG AT91C_US_CLKS_CLOCK   | \
                              AT91C_US_CHRL_8_BITS  | \
                              AT91C_US_PAR_NONE     | \
                              AT91C_US_OVER         | \
                              AT91C_US_NBSTOP_1_BIT | \
                              AT91C_US_MSBF


/** Enables the transmission over the RS485 interface
 *
 * @param baud_rate The required baud rate.
 * @param uart_mr Values for the uart's mode register.
 *        If 0 is specified, the DEFAULT_USART_MODEREG setting will be used.
 * @param timeout Reception timeout (bit periods). 0 means no timeout.
 * @param timeguard True enables the transmission timeguard.
 *        This feature enables the transmitter to insert an idle state
 *        between two characters. It's useful to interface with slow remote
 *        devices, since avoids remote overruns.
 *
 * @note By specifying 0 in the uart_mr parameter the default configuration
 * is mantained.
 *
 * @warning The RS485 driver should only be used if sensor port 4 has not
 * already been configured as a normal sensor port.
 */
void nx_rs485_init(nx_rs485_baudrate_t baud_rate,
                   U32 uart_mr,
                   U16 timeout,
                   bool timeguard);

/** Change the baud rate to a new value.
 *
 * This function is used when dealing with different or experimental
 * hardware interfaces.
 *
 * @param baud_rate The numerical value of required baud rate.
 * @return FALSE if the baud rate change cannot be achieved.
 */
bool nx_rs485_set_fixed_baudrate(U16 baud_rate);

/** Shuts down the peripheral
 *
 * @note Once the RS485 interface is shut down, port 4 may be used as a
 * normal sensor port again.
 *
 * @warning The RS485 driver should only be shutdown after all
 * in-progress transmissions have completed.
 */
void nx_rs485_shutdown(void);

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
bool nx_rs485_send(U8 *buffer, U32 buflen,
                   void (*callback)(nx_rs485_error_t));

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
bool nx_rs485_recv(U8 *buffer, U32 buflen,
                   void (*callback)(nx_rs485_error_t));

/** Aborts any underway transmission or pending reception
 *
 * @note The callback will be executed but not in an interrupt handler
 *       context. The callback's parameter will be RS485_ABORT.
 */
void nx_rs485_abort(void);

/*@}*/
/*@}*/

#endif /* __NXOS_BASE_DRIVERS_RS485_H__ */

