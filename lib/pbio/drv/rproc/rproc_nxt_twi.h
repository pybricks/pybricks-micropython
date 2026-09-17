// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2007 the NxOS developers
// Copyright (C) leJOS NXJ developers
// Copyright (c) 2025 The Pybricks Authors

// Two Wire Interface driver for the link to the NXT AVR coprocessor.

#ifndef _INTERNAL_PBDRV_RPROC_NXT_TWI_H_
#define _INTERNAL_PBDRV_RPROC_NXT_TWI_H_

#include <stdint.h>

/**
 * State of the TWI transfer most recently started.
 */
typedef enum {
    /** Idle, and the previous transfer completed normally. */
    PBDRV_RPROC_NXT_TWI_STATUS_READY,
    /** A transfer is in progress. */
    PBDRV_RPROC_NXT_TWI_STATUS_BUSY,
    /** Idle, but the previous transfer was aborted by a bus error. */
    PBDRV_RPROC_NXT_TWI_STATUS_ERROR,
} pbdrv_rproc_nxt_twi_status_t;

void pbdrv_rproc_nxt_twi_init(void);

void pbdrv_rproc_nxt_twi_write(const uint8_t *data, uint32_t len);

void pbdrv_rproc_nxt_twi_read(uint8_t *data, uint32_t len);

pbdrv_rproc_nxt_twi_status_t pbdrv_rproc_nxt_twi_get_status(void);

void pbdrv_rproc_nxt_twi_get_fault_info(uint32_t *fault_status, uint32_t *bytes_remaining);

#endif // _INTERNAL_PBDRV_RPROC_NXT_TWI_H_
