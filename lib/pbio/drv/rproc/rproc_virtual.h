// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_VIRTUAL_H_
#define _INTERNAL_PBDRV_RPROC_VIRTUAL_H_

#include <pbio/button.h>
#include <pbio/error.h>

void pbdrv_rproc_virtual_socket_send(const uint8_t *data, uint32_t size);

uint32_t pdrv_rproc_virtual_get_button_state(void);

#endif // _INTERNAL_PBDRV_RPROC_VIRTUAL_H_
