// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#ifndef _PBSYS_SYS_TELEMETRY_H_
#define _PBSYS_SYS_TELEMETRY_H_

#include <stdint.h>

#include <pbsys/config.h>


#if PBSYS_CONFIG_TELEMETRY

void pbsys_telemetry_init(void);
uint8_t *pbsys_telemetry_get_data(uint32_t *size);
void pbsys_telemetry_data_sent(void);

#else

static inline void pbsys_telemetry_init(void) {
}
static inline uint8_t *pbsys_telemetry_get_data(uint32_t *size) {
    *size = 0;
    return NULL;
}
static inline void pbsys_telemetry_data_sent(void) {
}

#endif // PBSYS_CONFIG_TELEMETRY

#endif // _PBSYS_SYS_TELEMETRY_H_
