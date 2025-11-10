// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#ifndef _PBSYS_SYS_TELEMETRY_H_
#define _PBSYS_SYS_TELEMETRY_H_

#include <pbsys/config.h>


#if PBSYS_CONFIG_TELEMETRY

void pbsys_telemetry_init(void);

#else

static inline void pbsys_telemetry_init(void) {
}

#endif // PBSYS_CONFIG_TELEMETRY

#endif // _PBSYS_SYS_TELEMETRY_H_
