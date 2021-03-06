// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_EVENT_H_
#define _PBIO_EVENT_H_

#include <stdint.h>

#include "pbio/port.h"

/**
 * Contiki process events.
 */
typedef enum {
    /** Command received from Pybricks BLE service. */
    PBIO_EVENT_COM_CMD,
    /** System status indicator was set. Data is ::pbio_pybricks_status_t. */
    PBIO_EVENT_STATUS_SET,
    /** System status indicator was cleared. Data is ::pbio_pybricks_status_t. */
    PBIO_EVENT_STATUS_CLEARED,
} pbio_event_t;

// TODO: these enums for Pybricks communication protocol should have their own header file

typedef enum {
    PBIO_COM_CMD_START_USER_PROGRAM     = 0,
    PBIO_COM_CMD_STOP_USER_PROGRAM      = 1,
} pbio_com_cmd_t;

typedef enum {
    PBIO_COM_MSG_TYPE_CMD               = 0,
} pbio_com_msg_type_t;

#endif // _PBIO_EVENT_H_
