/*
 * Copyright (c) 2018 David Lechner
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
