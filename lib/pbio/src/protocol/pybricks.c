// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Pybricks communication protocol

#include <stdint.h>

#include <pbio/protocol.h>
#include <pbio/util.h>

_Static_assert(NUM_PBIO_PYBRICKS_STATUS <= sizeof(uint32_t) * 8,
    "oh no, we added too many status flags");

/**
 * Writes Pybricks status report command to @p buf
 *
 * @param [in]  buf     The buffer to hold the binary data.
 * @param [in]  flags   The status flags.
 * @return              The number of bytes written to @p buf.
 */
uint32_t pbio_pybricks_event_status_report(uint8_t *buf, uint32_t flags) {
    buf[0] = PBIO_PYBRICKS_EVENT_STATUS_REPORT;
    pbio_set_uint32_le(&buf[1], flags);
    return 5;
}
