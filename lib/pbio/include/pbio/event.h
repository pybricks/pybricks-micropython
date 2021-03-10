// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef _PBIO_EVENT_H_
#define _PBIO_EVENT_H_

/**
 * Contiki process events.
 */
typedef enum {
    /** System status indicator was set. Data is ::pbio_pybricks_status_t. */
    PBIO_EVENT_STATUS_SET,
    /** System status indicator was cleared. Data is ::pbio_pybricks_status_t. */
    PBIO_EVENT_STATUS_CLEARED,
} pbio_event_t;

#endif // _PBIO_EVENT_H_
