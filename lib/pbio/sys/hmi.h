// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_HMI_H_
#define _PBSYS_SYS_HMI_H_

#include <pbsys/config.h>
#include <pbsys/status.h>

#define PBSYS_CONFIG_HMI_IDLE_TIMEOUT_MS (3 * 60000)

#if PBSYS_CONFIG_HMI

void pbsys_hmi_init(void);
void pbsys_hmi_deinit(void);

pbio_error_t pbsys_hmi_await_program_selection(void);

#else

static inline void pbsys_hmi_init(void) {
}
static inline void pbsys_hmi_deinit(void) {
}

static inline pbio_error_t pbsys_hmi_await_program_selection(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif // _PBSYS_SYS_HMI_H_
