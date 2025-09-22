// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBSYS_SYS_LIGHT_MATRIX_H_
#define _PBSYS_SYS_LIGHT_MATRIX_H_

#include <contiki.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HUB_LIGHT_MATRIX
void pbsys_hub_light_matrix_init(void);
void pbsys_hub_light_matrix_deinit(void);
void pbsys_hub_light_matrix_handle_user_program_start(void);
void pbsys_hub_light_matrix_update_program_slot(void);
#else
#define pbsys_hub_light_matrix_init()
#define pbsys_hub_light_matrix_deinit()
#define pbsys_hub_light_matrix_handle_user_program_start()
#define pbsys_hub_light_matrix_update_program_slot()
#endif

#endif // _PBSYS_SYS_LIGHT_MATRIX_H_
