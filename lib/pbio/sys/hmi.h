// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_HMI_H_
#define _PBSYS_SYS_HMI_H_

#include <contiki.h>

void pbsys_hmi_init();
void pbsys_hmi_handle_event(process_event_t event, process_data_t data);
void pbsys_hmi_poll();

#endif // _PBSYS_SYS_HMI_H_
