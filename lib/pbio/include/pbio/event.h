
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
