// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_HMI_H_
#define _PBSYS_SYS_HMI_H_

#include <pbsys/config.h>
#include <pbsys/status.h>

void pbsys_hmi_init(void);
void pbsys_hmi_handle_status_change(pbsys_status_change_t event, pbio_pybricks_status_t data);
void pbsys_hmi_poll(void);
pbio_error_t pbsys_hmi_await_program_selection(void);

#endif // _PBSYS_SYS_HMI_H_
