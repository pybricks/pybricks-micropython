// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#ifndef _PBSYS_SYS_TELEMETRY_H_
#define _PBSYS_SYS_TELEMETRY_H_

#include <stdint.h>

#include <pbsys/config.h>


#if PBSYS_CONFIG_TELEMETRY

uint32_t pbsys_telemetry_get_data(uint8_t *data, uint32_t max_size);

#else

static inline uint32_t pbsys_telemetry_get_data(uint8_t *data, uint32_t max_size) {
    return 0;
}

#endif // PBSYS_CONFIG_TELEMETRY

#endif // _PBSYS_SYS_TELEMETRY_H_
