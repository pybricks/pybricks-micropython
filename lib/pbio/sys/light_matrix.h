// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_LIGHT_MATRIX_H_
#define _PBSYS_SYS_LIGHT_MATRIX_H_

#include <contiki.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HUB_LIGHT_MATRIX
void pbsys_hub_light_matrix_init(void);
void pbsys_hub_light_matrix_handle_status_change(pbsys_status_change_t event, pbio_pybricks_status_t data);
void pbsys_hub_light_matrix_update_program_slot(void);
#else
#define pbsys_hub_light_matrix_init()
#define pbsys_hub_light_matrix_handle_status_change(event, data)
#define pbsys_hub_light_matrix_update_program_slot()
#endif

#endif // _PBSYS_SYS_LIGHT_MATRIX_H_
