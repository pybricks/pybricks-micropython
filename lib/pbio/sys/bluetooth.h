// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _PBSYS_SYS_BLUETOOTH_H_
#define _PBSYS_SYS_BLUETOOTH_H_

#include <stdint.h>

uint32_t pbsys_bluetooth_rx_get_free(void);
void pbsys_bluetooth_rx_write(const uint8_t *data, uint32_t size);
void pbsys_bluetooth_process_poll(void);

#endif // _PBSYS_SYS_BLUETOOTH_H_
