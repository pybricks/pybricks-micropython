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

void pbsys_status_set(pbio_pybricks_status_t status);
void pbsys_status_clear(pbio_pybricks_status_t status);
bool pbsys_status_test(pbio_pybricks_status_t status);
bool pbsys_status_test_debounce(pbio_pybricks_status_t status, bool state, uint32_t ms);
uint32_t pbsys_status_get_flags(void);

#endif // _PBSYS_STATUS_H_

/** @} */
