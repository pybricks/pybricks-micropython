/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _PBIO_EVENT_H_
#define _PBIO_EVENT_H_

#include <stdint.h>

#include "pbio/port.h"
#include "sys/process.h"

/**
 * Contiki process events.
 */
typedef enum {
    PBIO_EVENT_UART_RX,         /**< Character was received on a UART port. *data* is ::pbio_event_uart_rx_data_t. */
    PBIO_EVENT_COM_CMD,         /**< Command received from Pybricks BLE service */
} pbio_event_t;

/**
 * Data for ::PBIO_EVENT_UART_RX.
 */
typedef union {
    struct {
        pbio_port_t port;       /**< The port the UART is associated with. */
        uint8_t byte;           /**< The byte received. */
    };
    process_data_t data;        /**< For casting ::pbio_event_uart_rx_data_t to/from ::process_data_t */
} pbio_event_uart_rx_data_t;

// TODO: these enums for Pybricks communication protocol should have their own header file

typedef enum {
    PBIO_COM_CMD_START_USER_PROGRAM     = 0,
    PBIO_COM_CMD_STOP_USER_PROGRAM      = 1,
} pbio_com_cmd_t;

typedef enum {
    PBIO_COM_MSG_TYPE_CMD               = 0,
} pbio_com_msg_type_t;

#endif // _PBIO_EVENT_H_
