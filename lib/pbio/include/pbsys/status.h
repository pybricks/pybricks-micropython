// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

/**
 * @addtogroup SystemStatus System: Status
 * @{
 */

#ifndef _PBSYS_STATUS_H_
#define _PBSYS_STATUS_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/protocol.h>

#define PBSYS_STATUS_REPORT_SIZE 7

/**
 * Status flag change.
 */
typedef enum {
    /** System status indicator was set. */
    PBSYS_STATUS_CHANGE_SET,
    /** System status indicator was cleared. */
    PBSYS_STATUS_CHANGE_CLEARED,
} pbsys_status_change_t;

void pbsys_status_set_program_id(pbio_pybricks_user_program_id_t program_id);
void pbsys_status_set(pbio_pybricks_status_t status);
void pbsys_status_clear(pbio_pybricks_status_t status);
bool pbsys_status_test(pbio_pybricks_status_t status);
bool pbsys_status_test_debounce(pbio_pybricks_status_t status, bool state, uint32_t ms);
uint32_t pbsys_status_get_flags(void);
uint32_t pbsys_status_get_status_report(uint8_t *buf);

#endif // _PBSYS_STATUS_H_

/** @} */
