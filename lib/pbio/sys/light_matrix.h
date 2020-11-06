// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_LIGHT_MATRIX_H_
#define _PBSYS_SYS_LIGHT_MATRIX_H_

#include <contiki.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HUB_LIGHT_MATRIX
void pbsys_hub_light_matrix_init();
void pbsys_hub_light_matrix_handle_event(process_event_t event, process_data_t data);
#else
#define pbsys_hub_light_matrix_init()
#define pbsys_hub_light_matrix_handle_event(event, data)
#endif

#endif // _PBSYS_SYS_LIGHT_MATRIX_H_
